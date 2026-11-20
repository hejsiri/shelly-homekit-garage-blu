#!/usr/bin/env python3
"""Production Bind RPC and reading application with real Frozen and AES-CCM."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
mos=Path.home()/'.mos/work/shelly-garage-blu'
frozen=mos/'deps/modules/mongoose-os/src/frozen'
mbed=mos/'deps/modules/mbedtls/include'
crypto=Path.home()/'.mos/work/mbedtls-host/library/libmbedcrypto.a'
s=(root/'src/shelly_blu_pairing.cpp').read_text()
fn=s[s.index('void SetRoleMAC('):s.index('void CompleteUnpair(')]
g=(root/'src/shelly_blu_garage.cpp').read_text();a=g.index('void ApplyMiReading(');b=g.index('\nvoid Init()',a)
h=r'''
#include <cassert>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include "frozen.h"
#include "nvs.h"
#include "shelly_blu_keys.hpp"
#include "shelly_blu_mibeacon.hpp"
using namespace shelly::blu;
std::map<std::string,std::vector<uint8_t>> blobs;
std::string pending_name;std::vector<uint8_t> pending;bool erase_pending=false,fail_commit=false;
esp_err_t nvs_open(const char*,int,nvs_handle_t*h){*h=1;return 0;}
esp_err_t nvs_get_blob(nvs_handle_t,const char*n,void*p,size_t*l){auto it=blobs.find(n);if(it==blobs.end())return ESP_ERR_NVS_NOT_FOUND;assert(*l>=it->second.size());*l=it->second.size();memcpy(p,it->second.data(),*l);return 0;}
esp_err_t nvs_set_blob(nvs_handle_t,const char*n,const void*p,size_t l){pending_name=n;pending.assign((const uint8_t*)p,(const uint8_t*)p+l);erase_pending=false;return 0;}
esp_err_t nvs_erase_key(nvs_handle_t,const char*n){pending_name=n;erase_pending=true;return 0;}
esp_err_t nvs_commit(nvs_handle_t){if(fail_commit)return 1;if(erase_pending)blobs.erase(pending_name);else blobs[pending_name]=pending;return 0;}
void nvs_close(nvs_handle_t){}
struct mg_rpc_request_info{};struct mg_rpc_frame_info{};
struct mg_str{const char*p;size_t len;};
struct mgos_bt_addr{uint8_t addr[6];};
struct Candidate{mgos_bt_addr addr;int rssi;bool encrypted;uint16_t model;double seen;std::vector<uint8_t> beacon;};
std::vector<Candidate> candidates;
Sensor sensors[7];mgos_bt_addr addresses[7]={};bool known[7]={},configured=false;
unsigned packet_sequence[7]={};
std::string macs[7],state="scanning",error;
int discovery_model=0;
int role=-1,code=0,responses=0,restarts=0,mgos_sys_config;
bool authorized=true,discovery_bindkey=true,config_ok=true;
double now=100;
double mgos_uptime(){return now;}
bool Authorized(mg_rpc_request_info*){return authorized;}
bool ValidMAC(const char*s){return s&&strlen(s)==17;}
mg_str mg_mk_str(const char*s){return{s,strlen(s)};}
bool mgos_bt_addr_from_str(mg_str s,mgos_bt_addr*a){if(!ValidMAC(s.p))return false;unsigned v[6];if(sscanf(s.p,"%x:%x:%x:%x:%x:%x",v,v+1,v+2,v+3,v+4,v+5)!=6)return false;for(int i=0;i<6;i++)a->addr[i]=v[i];return true;}
const char *SensorMAC(int r){return macs[r].c_str();}
void mg_rpc_send_errorf(mg_rpc_request_info*,int c,const char*){code=c;}
void mg_rpc_send_responsef(mg_rpc_request_info*,const char*){responses++;}
bool mgos_sys_config_save(int*,bool,void*){return config_ok;}
namespace mgos {struct ScopedCPtr{void*p;explicit ScopedCPtr(void*v):p(v){}~ScopedCPtr(){free(p);}};void InvokeCB(std::function<void()>cb){cb();}}
void RestartService(){restarts++;}
void shelly::blu::ForgetReading(int r){sensors[r]=Sensor();known[r]=false;}
'''
for i,n in [(0,'closed'),(1,'open'),(2,'motion'),(3,'indoor'),(4,'outdoor'),(6,'smoke')]:h+=f'void mgos_sys_config_set_blu_{n}_mac(const char*s){{macs[{i}]=s;}}\n'
scan=g[g.index('void ScanResult('):g.index('\nvoid Scan(void *)')]
h+=g[g.index('void RestoreContactReadings()'):g.index('void ScanResult(')]+g[a:b]+fn+r'''
struct Buffer {const char*p;size_t len;};
struct mgos_bt_gap_scan_result {mgos_bt_addr addr;Buffer adv_data;Buffer scan_rsp;int rssi;};
void PairingAdvertisement(void*){} void PairingVerified(int){}
bool SensorReady(int r){return sensors[r].Fresh(now,180);}
bool PairingBusy(){return false;}
ButtonEvents button_events;ProgrammableButtonEvents programmable_button_events;
bool button_pending=false;unsigned button_press_sequence=0;double button_press_time=0;
std::function<void(uint8_t)> button_event_handler;
PLACEHOLDER_SCAN
std::vector<uint8_t> Hex(const char*s){std::vector<uint8_t>v;for(size_t i=0;s[i];i+=2){unsigned x;sscanf(s+i,"%2x",&x);v.push_back(x);}return v;}
std::string key="017e52d2684779298709b117c0a75a7b",mac="E4:AA:EC:81:21:AE";
void Reset(){for(int i=0;i<7;i++){assert(DeleteRecord(i));macs[i]="";sensors[i]=Sensor();known[i]=false;}state="scanning";code=responses=restarts=0;authorized=discovery_bindkey=config_ok=true;discovery_model=0;now=100;candidates.clear();mgos_bt_addr a;mgos_bt_addr_from_str(mg_mk_str(mac.c_str()),&a);candidates.push_back({a,-50,true,0x098b,now,Hex("58588b09a3ae2181ecaae40e2c553c040000d28a500c")});}
void Run(int r=0,const std::string&k=key){std::string json="{\"role\":"+std::to_string(r)+",\"mac\":\""+mac+"\",\"bindkey\":\""+k+"\"}";BindRPC(nullptr,nullptr,nullptr,{json.data(),json.size()});}
int main(){
 Reset();authorized=false;Run();assert(!responses&&code==0&&blobs.empty());
 for(auto k:{std::string(""),std::string(31,'a'),std::string(32,'g'),std::string(33,'a')}){Reset();Run(0,k);assert(code==400&&blobs.empty()&&!known[0]);}
 Reset();Run(2);assert(code==400&&blobs.empty());
 Reset();discovery_bindkey=false;Run();assert(code==400&&blobs.empty());
 Reset();now=116;Run();assert(code==400&&blobs.empty());
 Reset();Run(0,std::string(32,'0'));assert(code==400&&blobs.empty());
 Reset();candidates[0].beacon.back()^=1;Run();assert(code==400&&blobs.empty());
 Reset();macs[1]=mac;Run();assert(code==400&&blobs.empty());
 Reset();Run(3);assert(code==400&&blobs.empty());
 Reset();config_ok=false;Run();assert(code==500&&macs[0].empty()&&blobs.empty()&&!known[0]);
 Reset();Run();assert(responses==1&&code==0&&state=="done"&&known[0]&&sensors[0].seen&&!sensors[0].active&&macs[0]==mac);
 // Battery-only packets must never revive a stale endpoint or invent a contact.
 Reading battery;battery.battery=87;now=500;
 ApplyMiReading(0,addresses[0],battery,false,-42);
 assert(sensors[0].battery==87&&!sensors[0].Fresh(now,180)&&sensors[0].last_seen==100);
 Reading contact;contact.window_open=false;
 ApplyMiReading(0,addresses[0],contact,true,-42);
 assert(sensors[0].active&&sensors[0].Fresh(now,180));
 // Restore the real production snapshot without inventing a newly received packet.
 unsigned sequence=packet_sequence[0];now=510;sensors[0]=Sensor();
 RestoreContactReadings();
 assert(sensors[0].seen&&sensors[0].active&&sensors[0].last_seen==500);
 assert(packet_sequence[0]==sequence&&sensors[0].Fresh(now,180));
 now=681;assert(!sensors[0].Fresh(now,180));
 Reading opened;opened.window_open=true;ApplyMiReading(0,addresses[0],opened,true,-50);
 assert(!sensors[0].active&&sensors[0].last_seen==681);
 // Smoke pairing verifies an authenticated alarm state, never a door/motion role.
 mac="54:ef:44:e3:9c:bc";Reset();discovery_model=0x0997;
 candidates[0].model=0x0997;candidates[0].beacon=Hex("5859970966bc9ce344ef5401081205000000715ebe90");
 Run(6,"5b51a7c91cde6707c9ef18dfda143a58");
 assert(responses==1&&state=="done"&&known[6]&&sensors[6].seen&&sensors[6].active&&macs[6]==mac);
 Reading sb;sb.battery=72;now=500;ApplyMiReading(6,addresses[6],sb,false,-50);
 assert(sensors[6].active&&sensors[6].battery==72&&sensors[6].last_seen==500&&sensors[6].smoke_last_seen==100);
 Reading clear;clear.smoke=0;ApplyMiReading(6,addresses[6],clear,true,-50);
 assert(!sensors[6].active&&sensors[6].last_seen==500);
 // The physical test button can verify a key without fabricating an all-clear.
 Reset();discovery_model=0x0997;candidates[0].model=0x0997;
 candidates[0].beacon=Hex("5859970964bc9ce344ef5422206088fd000000003a148fb3");
 Run(6,"5b51a7c91cde6707c9ef18dfda143a58");
 assert(responses==1&&state=="done"&&known[6]&&sensors[6].seen&&!sensors[6].smoke_seen&&!sensors[6].active);
 // Unified LYWSD03MMC discovery permits authenticated Bind without a bindkey-only mode.
 mac="a4:c1:38:02:83:f4";Reset();discovery_bindkey=false;discovery_model=0x055b;
 candidates[0].model=0x055b;candidates[0].beacon=Hex("58585b0550f4830238c1a495ef58763c26000097e2abb5");
 Run(3,"e9ea895fac7cca6d30532432a516f3a8");assert(responses==1&&state=="done"&&sensors[3].seen&&sensors[3].humidity==47);
 // Stock LYWSD02MMC captures from Bluetooth-Devices/xiaomi-ble test_parser.py.
 mac="A4:C1:38:56:53:84";Reset();discovery_model=0x16e4;
 candidates[0].model=0x16e4;candidates[0].beacon=Hex("5858e4162c84535638c1a42b6ef2e91200006c884d9e");
 Run(0,"a115210eed7a88e50ad52662e732a9fb");assert(code==400&&!known[0]);
 Run(3,"a115210eed7a88e50ad52662e732a9fb");
 assert(responses==1&&XiaomiClockFor(3,addresses[3].addr)&&sensors[3].humidity==58&&!sensors[3].temperature_seen);
 mac="A4:C1:38:0E:FD:78";Reset();discovery_model=0x16e4;
 candidates[0].model=0x2542;candidates[0].beacon=Hex("585842258878fd0e38c1a405f6538aa72a0062b1a966");
 Run(4,"19b1c678ab0a8bc3dc77765f059188d4");assert(responses==1&&sensors[4].humidity==38&&!sensors[4].temperature_seen);
 auto temperature_packet=Hex("161695fe4858422529202d8c7a76b756a82a0078b8224e");
 mgos_bt_gap_scan_result clock={addresses[4],{nullptr,0},{reinterpret_cast<const char*>(temperature_packet.data()),temperature_packet.size()},-62};
 ScanResult(0,&clock,nullptr);assert(sensors[4].humidity==38&&sensors[4].temperature10==261);
 const auto clock_seq=packet_sequence[4];ScanResult(0,&clock,nullptr);assert(packet_sequence[4]==clock_seq);
 // Motion Sensor 2S: authenticated fixture must bind only to motion role.
 mac="dc:8e:95:2d:ea:43";Reset();discovery_model=0x3531;
 candidates[0].model=0x3531;candidates[0].beacon=Hex("485931350b64799117331ef4020000c5d2f6ac");
 Run(2,"685d647dc5e7bc9bcfcf5a1357bd2114");
 assert(responses==1&&state=="done"&&sensors[2].active&&sensors[2].illuminance100==5100);
 Reading mb;mb.battery=54;now=500;ApplyMiReading(2,addresses[2],mb,false,-40);
 assert(sensors[2].battery==54&&sensors[2].active&&sensors[2].last_seen==100);
 // Qingping encrypted motion and CGG1 use separate protocol records.
 mac="58:34:2d:01:02:03";Reset();discovery_bindkey=false;discovery_model=0x0a83;
 candidates[0].model=0x0a83;candidates[0].beacon=Hex("5858830a010302012d34580855b69d586d00000005c50e28");
 Run(2,"000102030405060708090a0b0c0d0e0f");assert(responses==1&&state=="done"&&sensors[2].active&&sensors[2].illuminance100==10000);
 SecureRecord qp;assert(LoadRecord(2,&qp)&&qp.protocol==kProtocolQingpingBindkey);
 Reading light;light.illuminance100=12000;now=500;ApplyMiReading(2,addresses[2],light,false,-40);
 assert(sensors[2].active&&sensors[2].last_seen==100&&!sensors[2].Fresh(now,180)&&sensors[2].illuminance_last_seen==500);
 mac="5a:58:2d:34:12:5f";Reset();discovery_model=0x0347;discovery_bindkey=false;
 candidates[0].model=0x0b48;candidates[0].beacon=Hex("5858480b685f12342d585a0b1841e2aa000e00a4964fb5");
 Run(3,"814aac74c4f17b6c1581e1ab87816b99");assert(responses==1&&state=="done"&&sensors[3].humidity==60&&!sensors[3].temperature_seen);
 mac="E4:AA:EC:81:21:AE";Reset();
 SecureRecord native={};native.version=1;native.protocol=kProtocolQingping;
 mgos_bt_addr addr=candidates[0].addr;memcpy(native.mac,addr.addr,6);native.key[0]=0x83;native.key[1]=0x0a;
 assert(SaveRecord(2,native));addresses[2]=addr;known[2]=true;
 std::vector<uint8_t> payload={8,0x12};for(int j=5;j>=0;--j)payload.push_back(addr.addr[j]);payload.insert(payload.end(),{8,4,1,100,0,0});
 std::vector<uint8_t> adv={uint8_t(payload.size()+3),0x16,0xcd,0xfd};adv.insert(adv.end(),payload.begin(),payload.end());
 mgos_bt_gap_scan_result packet={addr,{reinterpret_cast<const char*>(adv.data()),adv.size()},{nullptr,0},-44};
 ScanResult(0,&packet,nullptr);assert(sensors[2].seen&&sensors[2].active&&sensors[2].illuminance100==10000);
 payload.resize(8);payload.insert(payload.end(),{2,1,75});adv={uint8_t(payload.size()+3),0x16,0xcd,0xfd};adv.insert(adv.end(),payload.begin(),payload.end());packet.adv_data={reinterpret_cast<const char*>(adv.data()),adv.size()};now=500;
 ScanResult(0,&packet,nullptr);assert(sensors[2].battery==75&&!sensors[2].Fresh(now,180)&&sensors[2].last_seen==100);
 payload[1]=0x16;payload.resize(8);payload.insert(payload.end(),{1,4,0xd8,0,0xbb,1});adv={uint8_t(payload.size()+3),0x16,0xcd,0xfd};adv.insert(adv.end(),payload.begin(),payload.end());packet.adv_data={reinterpret_cast<const char*>(adv.data()),adv.size()};
 const auto seq=packet_sequence[2];ScanResult(0,&packet,nullptr);assert(packet_sequence[2]==seq); // Same MAC, wrong model.
 assert(DeleteRecord(2));ScanResult(0,&packet,nullptr);assert(packet_sequence[2]==seq);
 mac="E4:AA:EC:81:21:AE";Reset();fail_commit=true;Run();assert(code==500&&!known[0]&&macs[0].empty());fail_commit=false;
}
'''
h=h.replace('PLACEHOLDER_SCAN',scan)
with tempfile.TemporaryDirectory(prefix='blu-bind-rpc-') as t:
 p=Path(t);(p/'test.cpp').write_text(h)
 subprocess.run(['clang','-I',str(frozen),'-c',str(frozen/'frozen.c'),'-o',str(p/'frozen.o')],check=True)
 subprocess.run(['clang++','-std=c++14','-DSHELLY_GARAGE_BLE','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(root/'src'),'-I',str(root/'tests/stubs'),'-I',str(frozen),'-I',str(mbed),str(p/'test.cpp'),str(root/'src/shelly_blu_keys.cpp'),str(p/'frozen.o'),str(crypto),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: real Bind RPC authentication, key validation, MIC, role/MAC isolation, rollback, flash failure and contact freshness')
