#!/usr/bin/env python3
"""Test production GATT thermometer updates, freshness and endpoint isolation."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/shelly_blu_garage.cpp').read_text()
def extract(signature):
 start=source.index(signature);end=source.index('{',start)+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
harness=r'''
void RestoreContactReadings() {}
#include <cassert>
#include <cstring>
#include <string>
#include "shelly_blu_bthome.hpp"
using namespace shelly::blu;
struct mgos_bt_addr {uint8_t addr[6]={};};
struct mg_str {const char *p;};
mg_str mg_mk_str(const char *s){return {s};}
bool mgos_bt_addr_from_str(mg_str s,mgos_bt_addr *out){if(!*s.p)return false;out->addr[5]=uint8_t(s.p[16]-'0');return true;}
bool ValidMAC(const char *s){return s&&strlen(s)==17;}
int cleared_role=-1;bool null_mac=true;
const char *SensorMAC(int role){if(role==cleared_role)return null_mac?nullptr:"";return role==3?"00:00:00:00:00:03":role==4?"00:00:00:00:00:04":"";}
Sensor sensors[7];mgos_bt_addr addresses[7];bool known[7]={};int voltages[7]={};unsigned packet_sequence[7]={};
int timeout=180;double now=100;
int mgos_sys_config_get_blu_stale_after_s(){return timeout;}
double mgos_uptime(){return now;}
#define MGOS_BT_STARTED 1
bool bluetooth=true,scan=true,grace=false,pairing=false,configured=true;
int mgos_bt_get_status(){return bluetooth?MGOS_BT_STARTED:0;}
bool mgos_bt_gap_scan_in_progress(){return scan;}
bool XiaomiGATTScanPaused(){return grace;}
bool PairingBusy(){return pairing;}
bool QingpingFor(int,const unsigned char*){return false;}
bool XiaomiBindkeyFor(int,const unsigned char*){return false;}
bool SecureFor(int role,const uint8_t*){return role<3;}
bool XiaomiGATTFor(int role,const uint8_t*){return role==3;}
bool SensorPaired(int role){return role>=3;}
bool HTReady(int,bool);
'''
harness+='\n'.join(extract(signature) for signature in ['int SensorTimeout(', 'void SetThermometerReading(', 'bool HTReady(', 'bool Ready() {']).replace('blu::Ready(', 'shelly::blu::Ready(')
harness+=r'''
int main(){
 Reading endpoint;endpoint.window_open=false;sensors[0].Update(endpoint,now,-40);endpoint.window_open=true;sensors[1].Update(endpoint,now,-50);
 const Sensor closing=sensors[0],opening=sensors[1];
 Reading decoded;decoded.temperature_present=true;decoded.temperature10=213;decoded.humidity=52;Sensor reading;reading.Update(decoded,now,-47,2);
 mgos_bt_addr address;address.addr[5]=3;SetThermometerReading(3,address,reading,3000);
 assert(known[3]&&packet_sequence[3]==1&&voltages[3]==3000&&HTReady(3,true)&&HTReady(3,false));
 assert(sensors[0].active==closing.active&&sensors[0].last_seen==closing.last_seen&&sensors[1].active==opening.active&&sensors[1].last_seen==opening.last_seen);
 SetThermometerReading(0,address,Sensor(),0);address.addr[5]=4;SetThermometerReading(3,address,Sensor(),0);assert(packet_sequence[3]==1);
 assert(SensorTimeout(3)==3600&&SensorTimeout(4)==3600&&SensorTimeout(0)==180&&SensorTimeout(2)==180);
 timeout=1200;assert(SensorTimeout(3)==3600&&SensorTimeout(4)==3600&&SensorTimeout(0)==1200);timeout=180;
 sensors[4].Update(decoded,100,-50,2);
 now=3700;assert(HTReady(3,true)&&HTReady(3,false)&&HTReady(4,true)&&HTReady(4,false));
 // Battery beacons cannot prolong the age of either measurement.
 Reading battery; battery.battery=80;sensors[3].Update(battery,now,-47,2);
 now=3700.1;assert(!HTReady(3,true)&&!HTReady(3,false)&&!HTReady(4,true)&&!HTReady(4,false));
 // A newer temperature must not renew an old humidity value.
 Reading temperature;temperature.temperature_present=true;temperature.temperature10=225;
 sensors[3].Update(temperature,2000,-47,2);
 now=3701;assert(HTReady(3,true)&&!HTReady(3,false));
 now=5600;assert(HTReady(3,true));now=5600.1;assert(!HTReady(3,true));
 // The short connection grace does not authorize stale or conflicting endpoints.
 now=110;scan=false;grace=true;assert(Ready());pairing=true;assert(!Ready());pairing=false;
 sensors[1].active=true;assert(!Ready());sensors[1].active=false;
 now=281;assert(!Ready());now=110;grace=false;assert(!Ready());scan=true;assert(Ready());bluetooth=false;assert(!Ready()&&!HTReady(3,true));
 // A delayed reading after unpair must ignore a NULL or empty assigned MAC.
 for(bool nulls:{false,true})for(int selected:{3,4}) {
   cleared_role=selected;null_mac=nulls;address.addr[5]=selected;
   const auto seq=packet_sequence[selected];const auto seen=sensors[selected].last_seen;
   SetThermometerReading(selected,address,reading,3000);
   assert(packet_sequence[selected]==seq&&sensors[selected].last_seen==seen);
 }
 cleared_role=-1;
 puts("PASS: GATT freshness, isolated role updates and unchanged stale/conflicting endpoint checks");
}
'''
with tempfile.TemporaryDirectory(prefix='xiaomi-garage-') as temp:
 p=Path(temp);(p/'test.cpp').write_text(harness)
 subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
