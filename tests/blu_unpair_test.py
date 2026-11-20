#!/usr/bin/env python3
"""Exercise the production removal RPC: ACL, storage, role isolation and bonds."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/shelly_blu_pairing.cpp').read_text()
fn=s[s.index('void CompleteUnpair('):s.index('void PairRPC(')]
h=r'''
#define LOG(level,args) do {} while(0)
#include <atomic>
#include <cstdint>
#include <cctype>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <functional>
struct mg_rpc_request_info {}; struct mg_rpc_frame_info {};
struct mg_str {const char *p;int len;};
enum mgos_bt_addr_type {MGOS_BT_ADDR_TYPE_NONE=0};
struct mgos_bt_addr {unsigned char addr[6];enum mgos_bt_addr_type type;};
struct ble_addr_t {unsigned char addr[6];unsigned char type;};
constexpr int BLE_HS_ENOENT=5;
constexpr int kProtocolBTHome=0;
struct SecureRecord {int protocol=0;};
int protocol=0;
bool LoadRecord(int,SecureRecord*r){r->protocol=protocol;return true;}
std::string macs[7],names[7],car;
bool auth=true,active=false,key_ok=true,save_ok=true,unpair_busy=false,task_ok=true,null_empty=false;
double now=100;
double mgos_uptime(){return now;}
void (*queued_task)(void*)=nullptr;
constexpr int pdPASS=1,tskIDLE_PRIORITY=0;
int xTaskCreate(void(*task)(void*),const char*,int,void*,int,void*){if(!task_ok)return 0;queued_task=task;return pdPASS;}
void vTaskDelete(void*){}

int role=0,error=0,responses=0,deleted=-1,forgot=-1,gatt=-1,restarts=0,bonds=0,bond_result=0;
int mgos_sys_config;
bool Authorized(mg_rpc_request_info*){return auth;}
void json_scanf(const char*,int,const char*,int *r){*r=role;}
void mg_rpc_send_errorf(mg_rpc_request_info*,int code,const char*){error=code;}
void mg_rpc_send_responsef(mg_rpc_request_info*,const char*){responses++;}
bool Active(){return active;}
const char *SensorMAC(int i){return null_empty&&macs[i].empty()?nullptr:macs[i].c_str();}
REAL_VALID_MAC
mg_str mg_mk_str(const char*s){return {s,s?int(strlen(s)):0};}
REAL_BT_ADDRESS_PARSER
void mgos_bt_addr_to_esp32(mgos_bt_addr*a,ble_addr_t*b){memcpy(b->addr,a->addr,6);b->type=a->type;}
int DeleteTransportBond(const ble_addr_t&){bonds++;return bond_result;}
bool DeleteRecord(int i){deleted=i;return key_ok;}
void XiaomiGATTForget(int i){gatt=i;}
void SetSensorName(int i,const char*s){names[i]=s;}
void ForgetReading(int i){forgot=i;}
bool mgos_sys_config_save(int*,bool,void*){return save_ok;}
void RestartService(){restarts++;}
namespace mgos {void InvokeCB(std::function<void()> cb){cb();}}
'''
sdk=(Path.home()/'.mos/work/shelly-garage-blu/deps/bt-common/src/mgos_bt.c').read_text()
parser=sdk[sdk.index('bool mgos_bt_addr_from_str('):sdk.index('\nint mgos_bt_addr_cmp(')]
h=h.replace('REAL_BT_ADDRESS_PARSER',parser)
garage=(root/'src/shelly_blu_garage.cpp').read_text()
validator=garage[garage.index('bool ValidMAC('):garage.index('\nvoid Invalidate(')]
h=h.replace('REAL_VALID_MAC',validator)
for i,n in enumerate(['closed','open','motion','indoor','outdoor','button','smoke']):
 h+=f'void mgos_sys_config_set_blu_{n}_mac(const char*s){{macs[{i}]=s;}}\n'
h+='void mgos_sys_config_set_blu_car_name(const char*s){car=s;}\n'+fn+r'''
void Reset(){null_empty=false;unpair_busy=false;task_ok=true;now=100;queued_task=nullptr;transport_removal.request=nullptr;transport_removal.done=false;protocol=0;auth=true;active=false;key_ok=save_ok=true;role=error=responses=restarts=bonds=bond_result=0;deleted=forgot=gatt=-1;car="Auto";for(int i=0;i<7;i++){macs[i]="AA:BB:CC:DD:EE:00";macs[i][16]+=i;names[i]="Name";}}
mg_rpc_request_info request;
void Begin(){UnpairRPC(&request,nullptr,nullptr,{"",0});}
void Drain(){if(queued_task){auto task=queued_task;queued_task=nullptr;task(nullptr);}PollTransportRemoval();}
void Run(){Begin();Drain();}
int main(){
 // Empty config slots really are NULL on the device. Exercise the actual
 // SDK sscanf parser, not a forgiving mock that conceals this crash.
 for(int selected:{0,1,2,5})for(int empty=0;empty<7;++empty)if(empty!=selected) {
   Reset();role=selected;null_empty=true;macs[empty].clear();Run();
   assert(responses==1&&error==0&&deleted==selected&&bonds==2);
 }
 Reset();role=2;null_empty=true;macs[3].clear();macs[4].clear();Run();assert(responses==1&&deleted==2);
 // All 128 combinations, seven removal roles, and both empty representations.
 // Also covers already-unassigned slots and removal of orphaned app records.
 for(bool nulls:{false,true})for(int mask=0;mask<128;++mask)for(int selected=0;selected<7;++selected) {
   Reset();role=selected;null_empty=nulls;
   for(int i=0;i<7;++i)if(!(mask&(1<<i)))macs[i].clear();
   std::string before[7];for(int i=0;i<7;++i)before[i]=macs[i];
   Run();assert(responses==1&&error==0&&deleted==selected&&forgot==selected&&macs[selected].empty());
   assert(bonds==((mask&(1<<selected))?2:0));
   for(int i=0;i<7;++i)if(i!=selected)assert(macs[i]==before[i]&&names[i]=="Name");
 }
 // Successive removals create more NULL fields, exactly as real config does.
 Reset();null_empty=true;
 for(int selected=0;selected<7;++selected){role=selected;Run();assert(responses==selected+1&&error==0&&macs[selected].empty());}
 // Real validator must reject malformed optional slots before SDK parsing.
 Reset();role=2;macs[3]="zz:zz:zz:zz:zz:zz";macs[4]="broken";Run();assert(responses==1&&bonds==2);
 // No BLE storage operation or config mutation is allowed on the RPC stack.
 Reset();role=2;Begin();assert(unpair_busy&&queued_task&&bonds==0&&deleted==-1&&responses==0);
 PollTransportRemoval();assert(responses==0&&bonds==0);
 Begin();assert(error==409&&bonds==0);error=0;
 Drain();assert(!unpair_busy&&responses==1&&bonds==2&&deleted==2);
 // Task allocation failure preserves every application assignment.
 Reset();task_ok=false;Begin();assert(error==503&&!unpair_busy&&deleted==-1&&bonds==0);
 // Timeout replies once, preserves assignment and reserves the worker until exit.
 Reset();role=0;Begin();now=109;PollTransportRemoval();assert(error==504&&unpair_busy&&deleted==-1&&responses==0);
 error=0;PollTransportRemoval();assert(error==0);Begin();assert(error==409);
 Drain();assert(!unpair_busy&&deleted==-1&&responses==0&&!macs[0].empty());
 Reset();auth=false;Run();assert(deleted==-1&&responses==0);
 Reset();role=7;Run();assert(error==400&&deleted==-1);
 Reset();active=true;Run();assert(error==409&&deleted==-1);
 Reset();bond_result=99;Run();assert(error==500&&deleted==-1&&!macs[0].empty());
 Reset();key_ok=false;Run();assert(error==500&&forgot==-1&&!macs[0].empty());
 for(int r=0;r<6;r++) {Reset();role=r;Run();assert(responses==1&&error==0&&deleted==r&&forgot==r&&gatt==r&&macs[r].empty()&&names[r].empty());assert(restarts==1);assert(bonds==2);for(int i=0;i<7;i++)if(i!=r)assert(!macs[i].empty()&&names[i]=="Name");if(r==5)assert(car.empty());else assert(car=="Auto");}
 // Xiaomi passive, GATT and bindkey assignments must never touch NimBLE bonds,
 // even when that transport API would fail or block. Other roles remain intact.
 for(int p:{1,4,5,6,7})for(int r:{0,1,2,3,4,6}) {Reset();protocol=p;role=r;bond_result=99;Run();assert(bonds==0&&responses==1&&deleted==r&&forgot==r&&macs[r].empty());for(int i=0;i<7;i++)if(i!=r)assert(!macs[i].empty());}
 Reset();macs[1]=macs[0];Run();assert(bonds==0&&responses==1&&!macs[1].empty());
 Reset();bond_result=BLE_HS_ENOENT;Run();assert(responses==1);
 Reset();save_ok=false;Run();assert(error==500&&responses==0&&forgot==0);
}
'''
with tempfile.TemporaryDirectory(prefix='blu-unpair-') as t:
 p=Path(t);(p/'test.cpp').write_text(h)
 subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: 1792 slot/role/NULL combinations, sequential removals, real SDK parser/validator; asynchronous unpair, no BLE on RPC stack, 8s timeout, overlap rejection, late completion, allocation/storage failures, seven roles and shared bonds')
