#!/usr/bin/env python3
"""Exercise production GATT key callbacks, retries and cancellation with a fake peer."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/shelly_blu_pairing.cpp').read_text()
def extract(signature):
    start = source.index(signature)
    brace = source.index('{', start)
    end, depth = brace + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

harness = r'''
#include <array>
#include <atomic>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>
#define SHELLY_GARAGE_BLE
#include "shelly_blu_keys.hpp"
#include "shelly_blu_xiaomi.hpp"
#include "shelly_blu_xiaomi_gatt.hpp"
#define BLE_HS_ATT_ERR(x) (0x100 + (x))
#define BLE_ATT_ERR_INSUFFICIENT_AUTHEN 5
#define BLE_ATT_ERR_INSUFFICIENT_ENC 15
#define BLE_HS_ENOTCONN 7
#define BLE_HS_CONN_HANDLE_NONE 0xffff
#define BLE_ERR_REM_USER_CONN_TERM 0x13
#define BLE_HS_ERR_HCI_BASE 0x200
#define BLE_HS_EDONE 14
#define BLE_GATT_CHR_PROP_WRITE 8
#define BLE_SM_PAIR_KEY_DIST_ENC 1
#define MGOS_BT_ADDR_STR_LEN 18
struct Mbuf { std::vector<unsigned char> data; };
struct ble_gatt_error { int status; };
struct ble_gatt_attr { Mbuf *om; };
struct ble_uuid_t { std::string text; };
struct ble_uuid128_t { ble_uuid_t u; };
struct ble_gatt_chr { uint16_t val_handle; uint8_t properties; ble_uuid128_t uuid; };
ble_uuid128_t UUID(const char *text) { return {{text}}; }
int ble_uuid_cmp(const ble_uuid_t *a,const ble_uuid_t *b) { return a->text.compare(b->text); }
int OS_MBUF_PKTLEN(Mbuf *m) { return m->data.size(); }
int os_mbuf_copydata(Mbuf *m, int, int n, void *out) {
 if (int(m->data.size()) != n) return -1;
 memcpy(out, m->data.data(), n); return 0;
}
namespace mgos {
std::vector<std::function<void()>> callbacks;
void InvokeCB(std::function<void()> cb) { callbacks.push_back(cb); }
struct ScopedCPtr { void *p; explicit ScopedCPtr(void *value):p(value){} ~ScopedCPtr(){free(p);} };
std::string SPrintf(const char *fmt, ...) {
 char b[256]; va_list args; va_start(args,fmt); vsnprintf(b,sizeof(b),fmt,args); va_end(args); return b;
}
}
struct Timer { void (*cb)(void *); void *arg; };
std::vector<Timer> timers;
int mgos_set_timer(int delay, int flags, void (*cb)(void *), void *arg) {
 assert(delay == 500 && flags == 0); timers.push_back({cb,arg}); return 1;
}
int reads = 0, terminations = 0, writes = 0, saved = 0, reset_readings = 0;
int key_writes=0;
std::vector<uint8_t> written_key;
int ble_gap_terminate(int, int) { terminations++; return 0; }
using ReadCallback = int (*)(uint16_t, const ble_gatt_error *, ble_gatt_attr *, void *);
int ble_gattc_read(int, int, ReadCallback, void *) { reads++; return 0; }
int ble_gattc_write_flat(int,int,const void *data,int size,ReadCallback,void *) {
 key_writes++;const auto *p=static_cast<const uint8_t *>(data);written_key.assign(p,p+size);return 0;
}
void esp_fill_random(void *out,size_t size) { for(size_t i=0;i<size;i++)static_cast<uint8_t *>(out)[i]=0xa0+i; }
struct mgos_bt_addr { unsigned char addr[6]; int type=0; };
using Address = mgos_bt_addr;
Address peer = {{1,2,3,4,5,6},0};
Address native_peer = {};
struct mg_str { const char *p; size_t len; };
mg_str mg_mk_str(const char *s) { return {s,strlen(s)}; }
bool ValidMAC(const char *s) { return s && strlen(s)==17; }
void mgos_bt_addr_from_str(mg_str text,Address *out) {
 unsigned b[6];assert(sscanf(text.p,"%x:%x:%x:%x:%x:%x",b,b+1,b+2,b+3,b+4,b+5)==6);
 for(int i=0;i<6;i++)out->addr[i]=b[i];
}
std::string indoor_mac, outdoor_mac;
const char *SensorMAC(int role) { return role==3?indoor_mac.c_str():role==4?outdoor_mac.c_str():""; }
struct mg_rpc_request_info {};
struct mg_rpc_frame_info {};
bool authorized=true;
bool Authorized(mg_rpc_request_info *) { return authorized; }
int rpc_error=0,responses=0,requested_model=0,scan_stops=0;
const int MGOS_BT_STARTED=1;
int mgos_bt_get_status(){return MGOS_BT_STARTED;}
void mgos_bt_gap_scan_stop(){scan_stops++;}
void mg_rpc_send_errorf(mg_rpc_request_info *,int code,const char *,...) { rpc_error=code; }
void mg_rpc_send_responsef(mg_rpc_request_info *,const char *,...) { responses++; }
struct {int role;const char *mac,*pin,*current_pin;} request;
void json_scanf(const char *,size_t,const char *fmt,...) {
 va_list args;va_start(args,fmt);
 if(strcmp(fmt,"{model:%d}")==0) {
  *va_arg(args,int *)=requested_model;
 } else if(strcmp(fmt,"{pin:%Q}")==0) {
  *va_arg(args,char **)=request.pin?strdup(request.pin):nullptr;
 } else {
  assert(strcmp(fmt,"{role:%d,mac:%Q,pin:%Q,current_pin:%Q}")==0);
  *va_arg(args,int *)=request.role;
  for(const char *s : {request.mac,request.pin,request.current_pin})
   *va_arg(args,char **)=s?strdup(s):nullptr;
 }
 va_end(args);
}
struct mgos_bt_gap_scan_result { Address addr; int rssi; mg_str scan_rsp; mg_str adv_data={}; };
#define MGOS_BT_GAP_EIR_MANUFACTURER_SPECIFIC_DATA 0xff
mg_str mgos_bt_gap_parse_adv_data(mg_str response,int) { return response; }
void mgos_bt_addr_to_esp32(Address *addr,Address *out) { *out=*addr; }
struct Candidate { Address addr; int rssi; bool encrypted; uint16_t model; double seen; };
std::vector<Candidate> candidates;
void mgos_bt_addr_to_str(const Address *, int, char *out) { strcpy(out,"01:02:03:04:05:06"); }
void mgos_sys_config_set_blu_closed_mac(const char *) {}
void mgos_sys_config_set_blu_open_mac(const char *) {}
void mgos_sys_config_set_blu_motion_mac(const char *) {}
void mgos_sys_config_set_blu_indoor_mac(const char *s) { indoor_mac=s; }
void mgos_sys_config_set_blu_outdoor_mac(const char *s) { outdoor_mac=s; }
void mgos_sys_config_set_blu_button_mac(const char *) {}
int mgos_sys_config;
bool config_ok=true,record_ok=true;
bool mgos_sys_config_save(int *, bool, void *) { return config_ok; }
void shelly::blu::ResetReadings() { reset_readings++; }
namespace shelly { namespace blu {
XiaomiGATTCallback probe;
bool XiaomiGATTStart(int,const mgos_bt_addr &,XiaomiGATTCallback cb) { probe=cb;return true; }
void XiaomiGATTCancel() { probe=nullptr; }
void SetThermometerReading(int,const mgos_bt_addr &,const Sensor &,int) {}
}}
void RestoreSecurity() {}
double now=100;
double mgos_uptime() { return now; }
namespace shelly { namespace blu {
SecureRecord persisted = {}, previous = {};
bool have_previous = false;
bool LoadRecord(int, SecureRecord *out) { *out = previous; return have_previous; }
bool SaveRecord(int, const SecureRecord &record) { if(!record_ok)return false;persisted = record;saved++;return true; }
}}
using namespace shelly::blu;
std::atomic<unsigned> generation(1);
std::atomic<uint32_t> passkey(0);
std::string state, error;
std::vector<std::string> traces;
unsigned key_read_attempts = 0, empty_key_reads = 0, security_ticket = 0;
unsigned connection_attempts=0;
double attempt_started=0;
std::string diagnostics;
bool security_changed=false;
int saved_io=0,saved_bonding=0,saved_mitm=0,saved_our_keys=0,saved_their_keys=0;
bool security_bond_resume=false;
bool security_pairing_started=false,security_retry_used=false;
enum class BondRenewal { None, WaitingForPairingMode, Ready, Used };
BondRenewal bond_renewal=BondRenewal::None;
int connections = 0;
void Connect() { connections++; state = "connecting"; }
#define BLE_SM_IO_CAP_KEYBOARD_ONLY 2
#define BLE_SM_IO_CAP_NO_IO 3
struct { int sm_io_cap=0,sm_mitm=0,sm_bonding=0,sm_our_key_dist=0,sm_their_key_dist=0,sm_sc=1; } ble_hs_cfg;
bool existing = false;
bool rotated = false, key_reconnect_used = false;
bool key_writable=false,key_provision_used=false;
std::array<uint8_t,16> pending_key={};
uint32_t new_pin = 123456;
int role = 2;
int discovery_model=0;
uint16_t conn = 1, key_handle = 10, pin_handle=11,beacon_handle=0;
double deadline = 0;
bool Active() { return state != "idle" && state != "done" && state != "error"; }
void Finish(const char *message) { generation++; state = message ? "error" : "done"; error = message ? message : ""; }
int service_restarts=0;
void RestartService() { service_restarts++; }
void Trace(const std::string &message) { traces.push_back(message); }
void FailStart(int result, const char *message) { if (result) Finish(message); }
void SetPIN() { writes++; }
void ReadKey();
void QueueKeyRead();
void StoreKey(const std::array<uint8_t,16> &key);
'''
harness += '\n'.join(extract(sig) for sig in [
    'bool ResumeSecurityAfterDisconnect(', 'void WaitForSecurityRetryWake() {', 'bool ResumeKeyAfterDisconnect() {', 'void ConfigureReconnectAuthentication(', 'bool ResumePairingWindow() {', 'void WaitForUserPairing() {', 'int WriteResult(', 'int KeyWriteResult(', 'void ConfigureEncryption() {', 'int KeyResult(', 'void StoreKey(const std::array<uint8_t, 16> &key) {', 'void QueueKeyRead() {', 'void ReadKey() {', 'int Characteristic(', 'bool ParsePIN(', 'void EncryptionPINRPC(', 'void DiscoverRPC(', 'void StoreXiaomi(', 'void PairRPC(', 'void PairingAdvertisement(', 'bool PairingScan() {', 'void PairingVerified(', 'void PairingTimeout() {'])
harness += r'''
void Drain() {
 while (!mgos::callbacks.empty()) {
  auto callbacks = mgos::callbacks; mgos::callbacks.clear();
  for (auto &cb : callbacks) cb();
 }
}
void Tick() {
 auto pending = timers; timers.clear();
 for (auto &timer : pending) timer.cb(timer.arg);
 Drain();
}
void Reset() {
 config_ok=record_ok=true;indoor_mac.clear();outdoor_mac.clear();
 generation++; state = "reading_key"; error.clear(); traces.clear();requested_model=scan_stops=0;
 now=100;discovery_model=0;authorized=true;rpc_error=responses=service_restarts=0;connection_attempts=0;candidates.clear();
 request={1,"01:02:03:04:05:06",nullptr,nullptr};
 reads = writes = terminations = saved = reset_readings = 0;
 key_writes=0;written_key.clear();key_writable=key_provision_used=false;pending_key.fill(0);pin_handle=11;
 key_read_attempts = empty_key_reads = 0; key_reconnect_used = rotated = false;
 connections=0; existing=false; passkey.store(0); ble_hs_cfg={};
 security_bond_resume=false;bond_renewal=BondRenewal::None;
 security_pairing_started=security_retry_used=false;
 have_previous = false; new_pin = 123456; conn = 1;
 mgos::callbacks.clear(); timers.clear(); persisted = {};
}
void Reply(int status, int length, unsigned current = generation.load()) {
 Mbuf m;
 if (length >= 0) for (int i = 0; i < length; i++) m.data.push_back(i + 1);
 ble_gatt_error err{status}; ble_gatt_attr attr{length < 0 ? nullptr : &m};
 KeyResult(1,&err,&attr,reinterpret_cast<void *>(uintptr_t(current))); Drain();
}
void Advertise(bool pairing,bool auth,bool selected=true) {
 const char data[]={char(0xa9),0x0b,1,char((pairing?0x10:0)|(auth?2:0)),0,0x0b,1,0};
 mgos_bt_gap_scan_result result{peer,-50,{data,sizeof(data)}};
 if(!selected)result.addr.addr[5]^=1;
 PairingAdvertisement(&result);
}
int main() {
 // Withdrawn manual-key models are rejected even by an older cached panel.
 for(int model:{0x098b}) {
  Reset();state="idle";requested_model=model;DiscoverRPC(nullptr,nullptr,nullptr,{});
  assert(rpc_error==400&&state=="idle"&&scan_stops==0&&responses==0);
  for(int slot=0;slot<6;slot++) {
   Reset();state="scanning";request.role=slot;
   candidates.push_back({peer,-45,false,static_cast<uint16_t>(model),now});
   PairRPC(nullptr,nullptr,nullptr,{});assert(rpc_error==400&&saved==0&&connections==0);
  }
 }
 for(int model:{0,3,0x01aa,0x055b}) {
  Reset();state="idle";requested_model=model;DiscoverRPC(nullptr,nullptr,nullptr,{});
  assert(!rpc_error&&state=="scanning"&&scan_stops==1&&responses==1&&deadline==now+60);
 }
 // Discovery accepts real MiBeacon data only in the explicitly selected Xiaomi mode.
 Reset();state="scanning";
 const char xadv[]={25,0x16,char(0x95),char(0xfe),0x50,0x20,char(0xaa),1,3,6,5,4,3,2,1,0x0d,0x10,4,char(0xb4),0,char(0x95),2,0x0a,0x10,1,59};
 mgos_bt_gap_scan_result xr{peer,-55,{}, {xadv,sizeof(xadv)}};
 PairingAdvertisement(&xr);assert(candidates.empty());
 discovery_model=0x01aa;PairingAdvertisement(&xr);
 assert(candidates.size()==1&&candidates[0].model==0x01aa&&!candidates[0].encrypted);
 now++;PairingAdvertisement(&xr);assert(candidates.size()==1&&candidates[0].seen==now);
 // GATT discovery accepts stock encrypted headers but never treats them as readings.
 Reset();state="scanning";discovery_model=0x055b;
 const char gattadv[]={7,0x16,char(0x95),char(0xfe),0x58,0x58,0x5b,0x05};
 mgos_bt_gap_scan_result gr{peer,-55,{}, {gattadv,sizeof(gattadv)}};
 PairingAdvertisement(&gr);assert(candidates.size()==1&&candidates[0].model==0x055b&&!candidates[0].encrypted);
 Reset();state="scanning";discovery_model=0x055b;request.role=4;
 candidates.push_back({peer,-45,false,0x055b,now});PairRPC(nullptr,nullptr,nullptr,{});
 assert(!rpc_error&&state=="reading_xiaomi"&&saved==0&&probe);
 {auto cb=probe;cb(Sensor(),-1,"read failed");} assert(state=="error"&&saved==0);
 Reset();state="scanning";discovery_model=0x055b;request.role=4;
 candidates.push_back({peer,-45,false,0x055b,now});PairRPC(nullptr,nullptr,nullptr,{});
 {auto cb=probe;Sensor value;value.seen=true;cb(value,3000,"");}Drain();
 assert(state=="done"&&saved==1&&persisted.protocol==kProtocolXiaomiGATT&&service_restarts==1);
 for(int slot:{0,1,2,5}) {Reset();state="scanning";request.role=slot;discovery_model=0x055b;
 candidates.push_back({peer,-45,false,0x055b,now});PairRPC(nullptr,nullptr,nullptr,{});assert(rpc_error==400&&saved==0);}
 // A failed probe or storage operation cannot replace a previous thermometer.
 for(int mode:{0,1,2}) {
 Reset();state="scanning";discovery_model=0x055b;request.role=4;outdoor_mac="AA:BB:CC:DD:EE:FF";
 candidates.push_back({peer,-45,false,0x055b,now});PairRPC(nullptr,nullptr,nullptr,{});
 config_ok=mode!=1;record_ok=mode!=2;
 {auto cb=probe;Sensor value;value.seen=true;cb(value,3000,mode==0?"read failed":"");}Drain();
 assert(state=="error"&&saved==0&&outdoor_mac=="AA:BB:CC:DD:EE:FF"&&service_restarts==0);
 }
 // A cancelled session cannot commit through an old completion callback.
 Reset();state="scanning";discovery_model=0x055b;request.role=4;
 candidates.push_back({peer,-45,false,0x055b,now});PairRPC(nullptr,nullptr,nullptr,{});
 {auto cb=probe;Finish("cancelled");cb(Sensor(),3000,"");}assert(saved==0&&state=="error");
 // Xiaomi assigns only an H&T slot without touching BLE security or any relay.
 Reset();state="scanning";discovery_model=0x01aa;request.role=3;
 candidates.push_back({peer,-45,false,0x01aa,now});
 PairRPC(nullptr,nullptr,nullptr,{});
 assert(!rpc_error&&responses==1&&connections==0&&saved==1&&state=="waiting_beacon");
 assert(persisted.protocol==kProtocolXiaomi&&role==3);
 PairingVerified(4);assert(state=="waiting_beacon");
 PairingVerified(3);Drain();assert(state=="done"&&service_restarts==1);
 for(int slot:{0,1,2,5}) {
  Reset();state="scanning";request.role=slot;discovery_model=0x01aa;
  candidates.push_back({peer,-45,false,0x01aa,now});PairRPC(nullptr,nullptr,nullptr,{});
  assert(rpc_error==400&&saved==0&&connections==0);
 }
 Reset();state="scanning";request.role=3;discovery_model=3;
 candidates.push_back({peer,-45,false,0x01aa,now});PairRPC(nullptr,nullptr,nullptr,{});
 assert(rpc_error==400&&saved==0);
 // Pair accepts no new PIN; current PIN remains mandatory only for legacy auth.
 Reset();state="scanning";candidates.push_back({peer,-45,false,2,now});
 PairRPC(nullptr,nullptr,nullptr,{});
 assert(responses==1 && !rpc_error && new_pin==0 && passkey==0 && connections==1);
 Reset();state="scanning";candidates.push_back({peer,-45,true,2,now});
 PairRPC(nullptr,nullptr,nullptr,{});assert(rpc_error==400 && connections==0);
 rpc_error=0;request.current_pin="654321";PairRPC(nullptr,nullptr,nullptr,{});
 assert(!rpc_error && responses==1 && existing && passkey==654321 && new_pin==0);
 // Existing saved keys require a NEW verified packet from the selected role.
 Reset();state="scanning";candidates.push_back({peer,-45,false,2,now});
 have_previous=true;memcpy(previous.mac,peer.addr,6);
 PairRPC(nullptr,nullptr,nullptr,{});
 assert(state=="verifying_existing_key" && PairingScan() && connections==0);
 PairingVerified(0);assert(state=="verifying_existing_key");
 PairingVerified(1);Drain();assert(state=="done" && writes==0 && saved==0 && connections==0);
 // With no verified packet, wait for fresh pairing advertising, not stale discovery.
 Reset();state="scanning";candidates.push_back({peer,-45,false,2,now});
 have_previous=true;memcpy(previous.mac,peer.addr,6);
 PairRPC(nullptr,nullptr,nullptr,{});PairingTimeout();assert(state=="verifying_existing_key");
 now=103;PairingTimeout();assert(state=="waiting_pairing_mode" && connections==0);
 Advertise(true,false,false);Advertise(false,false);assert(connections==0);
 Advertise(true,false);assert(connections==1);
 Reset();state="verifying_existing_key";Finish("cancelled");PairingVerified(1);
 assert(state=="error");
 // A saved key for another peer is never used to bypass the selected peer's setup.
 Reset();state="scanning";candidates.push_back({peer,-45,false,2,now});
 have_previous=true;memcpy(previous.mac,peer.addr,6);previous.mac[5]^=1;
 PairRPC(nullptr,nullptr,nullptr,{});assert(state=="connecting" && connections==1);
 // Old panels can explicitly rotate a PIN; malformed or stale requests are rejected.
 for(const char *pin : {"123", "012345", "12x456", "1234567"}) {
  Reset();state="scanning";request.pin=pin;candidates.push_back({peer,-45,false,2,now});
  PairRPC(nullptr,nullptr,nullptr,{});assert(rpc_error==400 && connections==0);
 }
 Reset();state="scanning";request.pin="135790";candidates.push_back({peer,-45,false,2,now});
 PairRPC(nullptr,nullptr,nullptr,{});assert(!rpc_error && new_pin==135790 && connections==1);
 Reset();state="scanning";candidates.push_back({peer,-45,false,2,now-16});
 PairRPC(nullptr,nullptr,nullptr,{});assert(rpc_error==400 && connections==0);
 // Conditional encryption PIN RPC is authorized, validated and bound to a live stage.
 Reset();request.pin="135790";EncryptionPINRPC(nullptr,nullptr,nullptr,{});
 assert(rpc_error==409 && writes==0);
 Reset();state="waiting_encryption_pin";request.pin="135790";conn=BLE_HS_CONN_HANDLE_NONE;
 EncryptionPINRPC(nullptr,nullptr,nullptr,{});assert(rpc_error==409 && writes==0);
 Reset();state="waiting_encryption_pin";request.pin="135790";authorized=false;
 EncryptionPINRPC(nullptr,nullptr,nullptr,{});assert(writes==0 && responses==0);
 for(const char *pin : {"123", "012345", "12x456", "654321"}) {
  Reset();state="waiting_encryption_pin";request.pin=pin;passkey=654321;
  EncryptionPINRPC(nullptr,nullptr,nullptr,{});assert(rpc_error==400 && writes==0);
 }
 Reset();state="waiting_encryption_pin";request.pin="135790";
 EncryptionPINRPC(nullptr,nullptr,nullptr,{});assert(writes==1 && new_pin==135790 && responses==1);
 // Actual discovery must advertise WRITE (with response), not just WRITE_NO_RSP.
 for(int properties : {2,4,8,10}) {
  Reset();role=3;pin_handle=key_handle=0;new_pin=0;state="discovering_services";
  ble_gatt_chr characteristic{12,uint8_t(properties),UUID("eb0fb41b-af4b-4724-a6f9-974f55aba81a")};
  ble_gatt_error found{0},done{BLE_HS_EDONE};
  Characteristic(1,&found,&characteristic,reinterpret_cast<void *>(uintptr_t(generation.load())));Drain();
  assert(key_writable==bool(properties&8));
  Characteristic(1,&done,nullptr,reinterpret_cast<void *>(uintptr_t(generation.load())));Drain();
  assert(state=="reading_key" && reads==1 && writes==0);
 }
 // Peer-closed fresh pairing reconnects only to a new pairing advertisement
 // from the chosen device; it retains credentials and does not write a PIN.
 Reset();role=1;state="securing_link";security_pairing_started=true;passkey=654321;
 assert(ResumeSecurityAfterDisconnect(0x213) && PairingScan());
 assert(new_pin==123456 && passkey==654321 && security_retry_used);
 Advertise(true,false,false);Advertise(false,false);
 assert(connections==0 && writes==0 && saved==0);
 Advertise(true,false);assert(connections==1 && !PairingScan());
 assert(new_pin==123456 && passkey==654321 && writes==0 && saved==0);
 // A cancellation while waiting cannot be revived by a later advertisement.
 Reset();state="waiting_security_retry";Finish("cancelled");Advertise(true,false);
 assert(connections==0 && !PairingScan());
 // Physical pairing mode from the selected sensor resumes automatically,
 // without a panel confirmation. Other peers/ordinary advertisements cannot.
 Reset();role=1;state="securing_link";security_bond_resume=true;
 WaitForUserPairing();assert(bond_renewal==BondRenewal::WaitingForPairingMode);
 Advertise(true,false,false);Advertise(false,false);
 assert(connections==0 && bond_renewal==BondRenewal::WaitingForPairingMode);
 Advertise(true,false);assert(connections==1 && bond_renewal==BondRenewal::Ready);
 // Existing keys are read without changing PIN or key, even with write support.
 Reset();new_pin=0;key_writable=true;state="enabling_beacon";ble_gatt_error configured{0};
 WriteResult(1,&configured,nullptr,reinterpret_cast<void *>(uintptr_t(generation.load())));Drain();
 assert(reads==1 && writes==0 && key_writes==0);
 Reply(0,16);assert(state=="waiting_encrypted_beacon" && saved==1 && key_writes==0);
 // Only consistently empty key responses trigger automatic key provisioning.
 Reset();new_pin=0;key_writable=true;ReadKey();Reply(0,0);Tick();Reply(0,0);Tick();Reply(0,0);
 assert(state=="setting_key" && key_writes==1 && written_key.size()==16 && saved==0);
 assert(written_key.front()==0xa0 && written_key.back()==0xaf && writes==0);
 KeyWriteResult(1,&configured,nullptr,reinterpret_cast<void *>(uintptr_t(generation.load())));Drain();
 assert(saved==1 && state=="waiting_encrypted_beacon" && reads==3);
 assert(persisted.key[0]==0xa0 && persisted.key[15]==0xaf && (pending_key==std::array<uint8_t,16>{}));
 // Failed or cancelled writes cannot save an unconfirmed key.
 Reset();key_writable=true;ConfigureEncryption();ble_gatt_error rejected_key{0x103};
 KeyWriteResult(1,&rejected_key,nullptr,reinterpret_cast<void *>(uintptr_t(generation.load())));Drain();
 assert(state=="error" && saved==0);
 Reset();key_writable=true;ConfigureEncryption();unsigned key_generation=generation;
 Finish("cancelled");KeyWriteResult(1,&configured,nullptr,reinterpret_cast<void *>(uintptr_t(key_generation)));Drain();
 assert(saved==0 && state=="error");
 // Read-only legacy key attributes ask for a PIN only when activation is needed.
 Reset();new_pin=0;ReadKey();Reply(0,0);Tick();Reply(0,0);Tick();Reply(0,0);
 assert(state=="waiting_encryption_pin" && writes==0 && key_writes==0 && saved==0);
 // A malformed key followed by empty reads is not proof that encryption is off.
 Reset();new_pin=0;key_writable=true;ReadKey();Reply(0,8);Tick();Reply(0,0);Tick();Reply(0,0);
 assert(state=="error" && key_writes==0 && saved==0);
 // A modern Button1 reconnects with auth=0 even after PIN rotation. Unrelated
 // advertisements or a sensor outside pairing mode cannot resume this attempt.
 Reset();role=5;rotated=true;state="waiting_pairing_mode";
 Advertise(true,false,false);Advertise(false,false);
 assert(connections==0 && state=="waiting_pairing_mode");
 Advertise(true,false);assert(connections==1 && !existing && passkey==0);
 assert(ble_hs_cfg.sm_mitm==0 && writes==0 && saved==0);
 // Legacy authentication advertised by the selected peer still uses the new PIN.
 Reset();rotated=true;state="waiting_pairing_mode";
 Advertise(true,true);assert(connections==1 && existing && passkey==new_pin);
 assert(ble_hs_cfg.sm_mitm==1);
 // A pre-rotation reconnect must retain the original current PIN.
 Reset();existing=true;passkey.store(654321);state="waiting_pairing_mode";
 Advertise(true,true);assert(connections==1 && existing && passkey==654321);
 // A stalled endpoint security attempt waits for explicit physical confirmation.
 Reset(); role=1; state="securing_link"; passkey.store(654321);
 const unsigned stalled_generation=generation.load();
 WaitForUserPairing();
 assert(generation==stalled_generation+1 && conn==BLE_HS_CONN_HANDLE_NONE);
 assert(terminations==1 && state=="waiting_user_pairing" && deadline==280);
 assert(passkey==654321 && saved==0 && writes==0);
 assert(ResumePairingWindow() && state=="waiting_pairing_mode");
 // A lost post-PIN connection resumes once without rotating PIN or saving a key.
 Reset(); role = 2; rotated = true; state = "reading_key";
 const unsigned before = generation.load();
 Reply(BLE_HS_ENOTCONN,-1);
 assert(state == "reading_key" && timers.empty() && saved == 0);
 assert(ResumeKeyAfterDisconnect() && generation == before + 1);
 assert(connections == 1 && writes == 0 && saved == 0 && state == "connecting");
 state = "waiting_key_ready";
 assert(!ResumeKeyAfterDisconnect());
 // Door/Window, Button1 and H&T resume from a fresh pairing advertisement.
 // In particular ENOTCONN must not turn into an orphaned waiting_key_ready timer.
 for (int sensor : {0,1,3,4,5}) {
 Reset(); role = sensor; rotated = true; state = "reading_key";
 const unsigned button_generation = generation.load();
 Reply(BLE_HS_ENOTCONN,-1);
 assert(state == "reading_key" && timers.empty() && saved == 0);
 assert(ResumeKeyAfterDisconnect() && generation == button_generation + 1);
 assert(writes == 0 && saved == 0);
 assert(state == "waiting_user_pairing" && deadline == 280);
 assert(ResumePairingWindow() && state == "waiting_pairing_mode" && deadline == 160);
 assert(!ResumePairingWindow());
 // PIN rotation itself is not evidence that BLE PIN authentication is required.
 assert(!existing && passkey == 0 && ble_hs_cfg.sm_mitm == 0);
 ConfigureReconnectAuthentication(false);
 assert(!existing && passkey == 0 && ble_hs_cfg.sm_mitm == 0);
 assert(ble_hs_cfg.sm_io_cap == BLE_SM_IO_CAP_NO_IO);
 ConfigureReconnectAuthentication(true);
 assert(existing && passkey == new_pin && ble_hs_cfg.sm_mitm == 1);
 assert(ble_hs_cfg.sm_io_cap == BLE_SM_IO_CAP_KEYBOARD_ONLY);
 state = "waiting_key_ready";
 assert(!ResumeKeyAfterDisconnect());
 }
 Reset(); role = 5; rotated = false; state = "reading_key";
 assert(!ResumeKeyAfterDisconnect());
 Reset(); role = 0; rotated = true; state = "waiting_key_ready";
 assert(ResumeKeyAfterDisconnect() && state == "waiting_user_pairing");
 // A disconnect before the PIN write acknowledgement cannot imply success.
 Reset(); role = 1; state = "setting_pin";
 assert(!ResumeKeyAfterDisconnect());
 role = 2;
 // Configuration failures identify the operation and preserve its exact status.
 for (const char *stage : {"enabling_beacon", "setting_pin"}) {
  Reset(); state = stage; ble_gatt_error rejected{0x105};
  WriteResult(1,&rejected,nullptr,reinterpret_cast<void *>(uintptr_t(generation.load()))); Drain();
  assert(state == "error" && error.find("GATT 0x105") != std::string::npos);
  assert(error.find(stage == std::string("enabling_beacon") ? "beacon" : "PIN") != std::string::npos);
  assert(writes == 0 && reads == 0 && saved == 0);
 }
 // Delay after PIN rotation, and suppress a stale timer after cancellation.
 Reset(); state = "setting_pin";
 ble_gatt_error ok{0};
 WriteResult(1,&ok,nullptr,reinterpret_cast<void *>(uintptr_t(generation.load()))); Drain();
 assert(rotated && state == "waiting_key_ready" && reads == 0 && timers.size() == 1);
 generation++; Tick(); assert(reads == 0);
 // Three malformed responses (including missing mbuf) must never persist a key.
 Reset(); rotated = true; ReadKey();
 Reply(0,0); assert(state == "waiting_key_ready" && saved == 0); Tick();
 Reply(0,8); Tick(); Reply(0,-1);
 assert(state == "error" && reads == 3 && saved == 0 && error.find("length -1") != std::string::npos);
 // A transient response can recover, but only a full 16-byte key is accepted.
 Reset(); rotated = true; ReadKey(); Reply(0,8); Tick(); Reply(0,16);
 assert(saved == 1 && reads == 2 && state == "waiting_encrypted_beacon" && reset_readings == 1);
 assert(persisted.key[0] == 1 && persisted.key[15] == 16);
 // Oversized keys are rejected too; stale callbacks cannot save a late key.
 Reset(); ReadKey(); Reply(0,17); assert(saved == 0);
 Reset(); ReadKey(); const unsigned old_generation = generation.load();
 generation++; Reply(0,16,old_generation); assert(saved == 0);
 // Authentication failure permits only one fresh authenticated reconnect.
 Reset(); rotated = true; ReadKey(); Reply(BLE_HS_ATT_ERR(5),-1);
 assert(!key_reconnect_used && terminations == 1 && saved == 0);
 assert(state == "reconnecting_secure" && writes == 0);
 assert(ResumeKeyAfterDisconnect() && key_reconnect_used && state == "waiting_user_pairing");
 assert(!existing && connections==0);
 state = "reading_key"; key_read_attempts = 1; Reply(BLE_HS_ATT_ERR(15),-1);
 assert(state == "error" && terminations == 1 && saved == 0);
 // No PIN rotation means no attempt to reconnect using an unconfirmed new PIN.
 Reset(); ReadKey(); Reply(BLE_HS_ATT_ERR(5),-1);
 assert(state == "error" && !key_reconnect_used && terminations == 0 && saved == 0);
 // Re-reading an identical key must preserve the persistent replay counter.
 Reset(); have_previous = true; previous = {}; previous.version = 1;
 memcpy(previous.mac,peer.addr,6);
 for (int i = 0; i < 16; i++) previous.key[i] = i + 1;
 previous.counter_seen = 1; previous.counter = 1234;
 ReadKey(); Reply(0,16);
 assert(saved == 1 && persisted.counter == 1234 && persisted.counter_seen == 1);
 // After authenticated reconnection, beacon enable must not rotate PIN again.
 Reset(); rotated = true; state = "enabling_beacon";
 WriteResult(1,&ok,nullptr,reinterpret_cast<void *>(uintptr_t(generation.load()))); Drain();
 assert(reads == 1 && writes == 0);
}
'''
with tempfile.TemporaryDirectory(prefix='shelly-key-read-') as temp:
    d = Path(temp)
    (d / 'test.cpp').write_text(harness)
    subprocess.run(['clang++', '-std=c++14', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-I', str(root / 'src'), str(d / 'test.cpp'), '-o', str(d / 'test')], check=True)
    subprocess.run([str(d / 'test')], check=True)
print('PASS: endpoint key recovery, modern/legacy advertisements, bounded retries, strict key length, cancellation and replay preservation')
