#!/usr/bin/env python3
"""Exercise the production read-only Xiaomi GATT client with an asynchronous peer."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/shelly_blu_xiaomi_gatt.cpp').read_text()
source=source[source.index('namespace shelly {'):source.rindex('#endif')]
harness=r'''
#include <array>
#include <atomic>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>
#define SHELLY_GARAGE_BLE
#include "shelly_blu_xiaomi_gatt.hpp"
struct mgos_bt_addr { unsigned char addr[6]={}; };
struct mg_str { const char *p; size_t len; };
mg_str mg_mk_str(const char *s) {return {s,strlen(s)};}
struct ble_addr_t {unsigned char val[6]={};};
struct mgos_bt_uuid {std::string text;};
struct ble_uuid_t {std::string text;};
struct ble_uuid_any_t {ble_uuid_t u;};
struct Mbuf {std::vector<uint8_t> bytes;};
struct ble_gatt_error {int status;};
struct ble_gatt_attr {Mbuf *om;};
struct ble_gatt_chr {ble_uuid_any_t uuid;uint16_t val_handle;};
int ble_uuid_cmp(const ble_uuid_t *a,const ble_uuid_t *b){return a->text.compare(b->text);}
struct ble_gap_event {
 int type;
 struct {int status; uint16_t conn_handle;} connect;
 struct {struct {uint16_t conn_handle;} conn; int reason;} disconnect;
};
#define BLE_GAP_EVENT_CONNECT 1
#define BLE_GAP_EVENT_DISCONNECT 2
#define BLE_HS_CONN_HANDLE_NONE 0xffff
#define BLE_HS_EDONE 14
#define BLE_HS_ENOENT 5
#define BLE_ERR_REM_USER_CONN_TERM 0x13
#define MGOS_BT_STARTED 1
#define MGOS_TIMER_REPEAT 1
int OS_MBUF_PKTLEN(Mbuf *m){return m->bytes.size();}
int os_mbuf_copydata(Mbuf *m,int,int size,void *out){memcpy(out,m->bytes.data(),size);return 0;}
double now=0;
double mgos_uptime(){return now;}
namespace mgos {
std::vector<std::function<void()>> callbacks;
void InvokeCB(std::function<void()> cb){callbacks.push_back(cb);}
std::string SPrintf(const char *fmt,...){char b[512];va_list a;va_start(a,fmt);vsnprintf(b,sizeof(b),fmt,a);va_end(a);return b;}
std::string JSONPrintStringf(const char *,...){return "{}";}
}
void Drain(){while(!mgos::callbacks.empty()){auto batch=std::move(mgos::callbacks);mgos::callbacks.clear();for(auto &cb:batch)cb();}}
using Reader=int (*)(uint16_t,const ble_gatt_error *,ble_gatt_attr *,void *);
using Gap=int (*)(ble_gap_event *,void *);
using Chr=int (*)(uint16_t,const ble_gatt_error *,const ble_gatt_chr *,void *);
Chr chr_cb=nullptr;void *chr_arg=nullptr;
int handle_base=80,discovery_mask=31,discoveries=0,discover_result=0;
const char *expected_uuids[]={"2a24","2a26","2a27","ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6","8edfffef-3d1b-9c37-4623-ad7265f14076"};
Reader read_cb=nullptr;void *read_arg=nullptr;
Gap gap_cb=nullptr;void *gap_arg=nullptr;
std::vector<std::string> reads;
int connects=0,terminates=0,cancels=0,stops=0,delivered=0;
int start_result=0,read_result=0;
int ble_gap_connect(uint8_t,const ble_addr_t *,int timeout,void *,Gap cb,void *arg){assert(timeout==3000);connects++;gap_cb=cb;gap_arg=arg;return start_result;}
int ble_gap_terminate(uint16_t,int){terminates++;return 0;}
int ble_gap_conn_cancel(){cancels++;return 0;}
int ble_gap_conn_rssi(uint16_t,int8_t *out){*out=-47;return 0;}
int ble_hs_id_infer_auto(int,uint8_t *out){*out=0;return 0;}
void mgos_bt_addr_to_esp32(const mgos_bt_addr *a,ble_addr_t *b){memcpy(b->val,a->addr,6);}
void mgos_bt_gap_scan_stop(){stops++;}
void mgos_bt_uuid_from_str(mg_str s,mgos_bt_uuid *out){out->text=std::string(s.p,s.len);}
void mgos_bt_uuid_to_esp32(const mgos_bt_uuid *s,ble_uuid_any_t *out){out->u.text=s->text;}
int ble_gattc_disc_all_chrs(uint16_t h,int begin,int end,Chr cb,void *arg){assert(h==7&&begin==1&&end==0xffff);discoveries++;chr_cb=cb;chr_arg=arg;return discover_result;}
int ble_gattc_read(uint16_t h,uint16_t attribute,Reader cb,void *arg){assert(h==7&&attribute>handle_base&&attribute<=handle_base+5);reads.push_back(expected_uuids[attribute-handle_base-1]);read_cb=cb;read_arg=arg;return read_result;}
int mgos_bt_get_status(){return MGOS_BT_STARTED;}
int mgos_set_timer(int delay,int flags,void (*)(void *),void *){assert(delay==500&&flags==1);return 1;}
bool pairing=false;int updates=0;
namespace shelly {namespace blu {
bool PairingBusy(){return pairing;}
bool PairingScan(){return pairing;}
bool ValidMAC(const char *s){return s&&strlen(s)==17;}
const char *SensorMAC(int role){return role==3?"01:02:03:04:05:06":"";}
bool XiaomiGATTFor(int role,const uint8_t *){return role==3;}
void SetThermometerReading(int,const mgos_bt_addr &,const Sensor &s,int voltage){assert(s.seen&&voltage==3000);updates++;}
}}
bool mgos_bt_addr_from_str(mg_str,mgos_bt_addr *out){*out={};return true;}
'''
harness+=source+r'''
using namespace shelly::blu;
Sensor received;int mv=-1;std::string failure;
void Begin(){
 assert(XiaomiGATTStart(3,mgos_bt_addr(),[](const Sensor &s,int voltage,const std::string &error){delivered++;received=s;mv=voltage;failure=error;}));
}
void CharacteristicReply(int index){ble_gatt_error e{0};ble_gatt_chr c{{{expected_uuids[index]}},uint16_t(handle_base+index+1)};chr_cb(7,&e,&c,chr_arg);}
void DiscoveryDone(int status=BLE_HS_EDONE){ble_gatt_error e{status};chr_cb(7,&e,nullptr,chr_arg);Drain();}
void Connected(int result=0,bool discover=true){
 ble_gap_event event={};event.type=BLE_GAP_EVENT_CONNECT;event.connect={result,7};gap_cb(&event,gap_arg);Drain();
 if(!result&&discover&&busy){for(int index=4;index>=0;index--)if(discovery_mask&(1<<index))CharacteristicReply(index);DiscoveryDone();}
}
void Reply(const std::vector<uint8_t> &bytes,int status=0){
 auto cb=read_cb;auto arg=read_arg;ble_gatt_error e{status};Mbuf m{bytes};ble_gatt_attr a{&m};
 cb(7,&e,status?nullptr:&a,arg);
 Drain();
}
void Text(const char *s){Reply(std::vector<uint8_t>(s,s+strlen(s)));}
void CString(const char *s){Reply(std::vector<uint8_t>(s,s+strlen(s)+1));}
void Identify(){CString("LYWSD03MMC");CString("2.1.1_0159");CString("B1.4");}
void Reset(){
 XiaomiGATTCancel();XiaomiGATTSetGarageMoving(false);now=100;pairing=false;
 handle_base=80;discovery_mask=31;discoveries=discover_result=0;
 connects=terminates=cancels=stops=delivered=updates=0;start_result=read_result=0;reads.clear();failure.clear();status[0]=Status();status[1]=Status();cooldown=5;
}
int main(){
 Reading decoded;int voltage;
 uint8_t bytes[]={0xdd,0xfd,65,0xb8,0x0b}; // -5.47 C, 65%, 3000 mV.
 assert(DecodeXiaomi03GATT(bytes,5,&decoded,&voltage)&&decoded.temperature10==-55&&decoded.humidity==65&&decoded.battery==90&&voltage==3000);
 for(size_t size=0;size<5;size++)assert(!DecodeXiaomi03GATT(bytes,size,&decoded,&voltage));
 assert(!DecodeXiaomi03GATT(bytes,6,&decoded,&voltage));
 bytes[2]=101;assert(!DecodeXiaomi03GATT(bytes,5,&decoded,&voltage));bytes[2]=65;
 bytes[3]=bytes[4]=0;assert(!DecodeXiaomi03GATT(bytes,5,&decoded,&voltage));
 const uint8_t adv[]={7,0x16,0x95,0xfe,0x58,0x58,0x5b,0x05};
 assert(Xiaomi03Advertisement(adv,sizeof(adv)));
 for(size_t n=0;n<sizeof(adv);n++)assert(!Xiaomi03Advertisement(adv,n));
 // UUID lookup, mandatory identity, optional versions and a complete fresh sample.
 Reset();Begin();assert(XiaomiGATTConnecting()&&XiaomiGATTScanPaused());
 assert(!XiaomiGATTStart(4,mgos_bt_addr(),nullptr));Connected();assert(!XiaomiGATTConnecting());
 Identify();assert(reads.size()==4&&reads.back().find("ebe0ccc1")==0);
 Reply({0xdd,0xfd,65,0xb8,0x0b});
 assert(delivered==1&&failure.empty()&&received.temperature10==-55&&mv==3000&&terminates==1);
 assert(status[0].firmware=="2.1.1_0159"&&status[0].hardware=="B1.4"&&status[0].next==now+300);
 // A standard string may include its trailing NUL terminator.
 Reset();Begin();Connected();Reply({'L','Y','W','S','D','0','3','M','M','C',0});
 assert(busy&&step==1&&delivered==0);
 // Padding is normalized, while embedded NULs, prefixes and suffixes stay invalid.
 Reset();Begin();Connected();Reply({' ','L','Y','W','S','D','0','3','M','M','C',0,' ','\r','\n'});assert(busy&&step==1);
 for(const char *wrong:{"Other model","LYWSD03MMC-extra","xLYWSD03MMC"}) {
  Reset();Begin();Connected();CString(wrong);assert(delivered==1&&!failure.empty()&&failure.find("bytes ")!=std::string::npos&&failure.find("length ")!=std::string::npos);
 }
 Reset();Begin();Connected();Reply({'L','Y','W','S','D','0','3','M','M','C',0,'x'});assert(delivered==1&&!failure.empty());
 Reset();Begin();Connected();Reply(std::vector<uint8_t>(33,'x'));assert(delivered==1&&failure.find("length 33")!=std::string::npos);
 // Discovery order and changing attribute handles do not alter identity checks.
 Reset();handle_base=0x123;Begin();Connected();Identify();Reply({0,0,40,0xb8,0x0b});assert(delivered==1&&failure.empty()&&discoveries==1);
 Reset();discovery_mask=30;Begin();Connected();assert(delivered==1&&!failure.empty()&&reads.empty());
 Reset();Begin();Connected(0,false);CharacteristicReply(0);CharacteristicReply(0);DiscoveryDone();assert(delivered==1&&reads.empty());
 Reset();Begin();Connected(0,false);DiscoveryDone(0x105);assert(delivered==1&&failure.find("0x105")!=std::string::npos);
 // Startup errors and incomplete discovery remain bounded and report no sample.
 Reset();discover_result=6;Begin();Connected();assert(delivered==1&&!received.seen&&reads.empty()&&failure.find("0x6")!=std::string::npos);
 Reset();read_result=6;Begin();Connected();assert(delivered==1&&!received.seen&&failure.find("0x6")!=std::string::npos);
 Reset();Begin();Connected(0,false);now+=21;Tick(nullptr);assert(delivered==1&&!received.seen&&!busy);DiscoveryDone();assert(reads.empty());
 // Optional versions and a missing primary do not block a valid mirror read.
 Reset();discovery_mask=17;Begin();Connected();CString("LYWSD03MMC");assert(step==4&&reads.size()==2);Reply({0,0,40,0xb8,0x0b});assert(delivered==1&&failure.empty());
 // A cancelled discovery cannot begin reads on a reused connection.
 Reset();Begin();Connected(0,false);auto old_chr=chr_cb;auto old_chr_arg=chr_arg;XiaomiGATTCancel();Begin();Connected(0,false);
 {ble_gatt_error e{BLE_HS_EDONE};old_chr(7,&e,nullptr,old_chr_arg);}Drain();assert(busy&&reads.empty());XiaomiGATTCancel();
 // Wrong identity must never read data or produce measurements.
 Reset();Begin();Connected();Text("Other model");assert(delivered==1&&!received.seen&&!failure.empty()&&reads.size()==1);
 // Missing primary uses the documented mirror, without writing or enabling notifications.
 Reset();Begin();Connected();Identify();Reply({},0x10a);assert(reads.back().find("8edfffef")==0);Reply({0,0,40,0xb8,0x0b});assert(delivered==1&&failure.empty());
 // Authentication errors are reported; the client never initiates security.
 Reset();Begin();Connected();Identify();Reply({},0x105);Reply({},0x105);assert(delivered==1&&!failure.empty()&&reads.size()==5);
 // No silent zero values from invalid/empty data on either characteristic.
 Reset();Begin();Connected();Identify();Reply({0,0,0,0,0});Reply({});assert(delivered==1&&!received.seen&&!failure.empty());
 // Timeout, cancel, late callback and reused connection handle isolation.
 Reset();Begin();auto old_gap=gap_cb;auto old_arg=gap_arg;now+=19;Tick(nullptr);assert(delivered==0&&XiaomiGATTConnecting());now+=2;Tick(nullptr);assert(delivered==1&&cancels==1&&!XiaomiGATTConnecting());
 Begin();Connected();ble_gap_event late={};late.type=BLE_GAP_EVENT_DISCONNECT;late.disconnect={{7},0x208};old_gap(&late,old_arg);Drain();assert(busy);
 auto old_read=read_cb;auto old_read_arg=read_arg;XiaomiGATTCancel();Begin();Connected();ble_gatt_error done{BLE_HS_EDONE};old_read(7,&done,nullptr,old_read_arg);Drain();assert(busy&&step==0);XiaomiGATTCancel();
 Reset();Begin();XiaomiGATTCancel();Connected();assert(delivered==0&&terminates==1); // Late successful connection gets closed.
 Reset();Begin();Connected();XiaomiGATTSetGarageMoving(true);assert(delivered==1&&!failure.empty()&&!busy);assert(!XiaomiGATTStart(3,mgos_bt_addr(),nullptr));
 // Polling is serialized, deferred for movement/pairing, and never refreshes on failure.
 Reset();XiaomiGATTSetGarageMoving(true);Tick(nullptr);assert(connects==0);XiaomiGATTSetGarageMoving(false);
 pairing=true;Tick(nullptr);assert(connects==0);pairing=false;Tick(nullptr);assert(connects==1);Connected();Identify();Reply({0,0,40,0xb8,0x0b});assert(updates==1);now+=299;Tick(nullptr);assert(connects==1);now++;Tick(nullptr);assert(connects==2);Connected(5);assert(updates==1&&status[0].next==now+60);
 puts("PASS: stock Xiaomi GATT UUID reads, identity, decoding, fallback, timing, cancellation and BLE ownership");
}
'''
with tempfile.TemporaryDirectory(prefix='xiaomi-gatt-') as temp:
 p=Path(temp);(p/'test.cpp').write_text(harness)
 subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
