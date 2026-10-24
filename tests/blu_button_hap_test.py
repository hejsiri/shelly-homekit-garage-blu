#!/usr/bin/env python3
"""Compile the production BLE receive path and HomeKit services with host adapters."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def extract(signature):
    source = (root / 'src/shelly_blu_garage.cpp').read_text()
    start = source.index(signature); end = source.index('{', start) + 1; depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}'); end += 1
    return source[start:end]

harness = r'''
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "shelly_blu_bthome.hpp"
#include "shelly_blu_xiaomi.hpp"
using HAPError = int;
struct HAPAccessoryServerRef {};
struct HAPBoolCharacteristicReadRequest {};
struct HAPUInt8CharacteristicReadRequest {};
struct HAPFloatCharacteristicReadRequest {};
const int kHAPError_None=0,kHAPError_InvalidState=2;
const int kHAPServiceType_MotionSensor=1,kHAPServiceType_TemperatureSensor=2,
 kHAPServiceType_HumiditySensor=3,kHAPServiceType_OccupancySensor=4,kHAPServiceType_StatelessProgrammableSwitch=5,kHAPServiceType_LightSensor=6;
const int kHAPCharacteristicType_MotionDetected=1,kHAPCharacteristicType_StatusFault=2,
 kHAPCharacteristicType_CurrentTemperature=3,kHAPCharacteristicType_CurrentRelativeHumidity=4,
 kHAPCharacteristicType_OccupancyDetected=5,kHAPCharacteristicType_ProgrammableSwitchEvent=6,kHAPCharacteristicType_StatusActive=7,kHAPCharacteristicType_CurrentAmbientLightLevel=8;
const int kHAPCharacteristicUnits_Lux=1;
const char *kHAPServiceDebugDescription_LightSensor="light",*kHAPCharacteristicDebugDescription_CurrentAmbientLightLevel="lux";
const char *kHAPServiceDebugDescription_MotionSensor="motion",*kHAPServiceDebugDescription_TemperatureSensor="temperature",
 *kHAPServiceDebugDescription_HumiditySensor="humidity",*kHAPServiceDebugDescription_OccupancySensor="occupancy",
 *kHAPServiceDebugDescription_StatelessProgrammableSwitch="button";
const char *kHAPCharacteristicDebugDescription_MotionDetected="motion",*kHAPCharacteristicDebugDescription_StatusFault="fault",
 *kHAPCharacteristicDebugDescription_CurrentTemperature="temperature",*kHAPCharacteristicDebugDescription_CurrentRelativeHumidity="humidity",
 *kHAPCharacteristicDebugDescription_StatusActive="active",*kHAPCharacteristicDebugDescription_OccupancyDetected="occupancy",*kHAPCharacteristicDebugDescription_ProgrammableSwitchEvent="event";
const int MGOS_TIMER_REPEAT=1,MGOS_BT_STARTED=1;
int mgos_hap_accessory_information_service=0;
double now=10; bool bluetooth=true,scanning=true,authenticated=true,pairing=false,paired=false;
int mgos_bt_get_status(){return bluetooth?MGOS_BT_STARTED:0;}
bool mgos_bt_gap_scan_in_progress(){return scanning;}
double mgos_uptime(){return now;}
int mgos_sys_config_get_blu_presence_timeout_s(){return 45;}
std::string car_name="Tesla",button_name="Remote";
const char *mgos_sys_config_get_blu_car_name(){return car_name.c_str();}
namespace mgos {
struct Timer {
 static std::vector<Timer*> timers; std::function<void()> cb;
 explicit Timer(std::function<void()> fn):cb(std::move(fn)){timers.push_back(this);}
 ~Timer(){timers.erase(std::remove(timers.begin(),timers.end(),this),timers.end());}
 void Reset(int,int){}
};
std::vector<Timer*> Timer::timers;
namespace hap {
struct Characteristic {
 uint16_t iid; const int *type; int notifications=0;
 Characteristic(uint16_t id,const int *t):iid(id),type(t){}
 virtual ~Characteristic()=default;
 virtual void RaiseEvent(){notifications++;}
};
struct UInt8Characteristic : Characteristic {
 using Handler=std::function<HAPError(HAPAccessoryServerRef*,const HAPUInt8CharacteristicReadRequest*,uint8_t*)>;
 Handler read; std::vector<int> values;
 UInt8Characteristic(uint16_t id,const int *t,int,int,int,Handler handler,bool,std::nullptr_t,const char*)
  :Characteristic(id,t),read(std::move(handler)){}
 void RaiseEvent() override {notifications++;uint8_t v=0;if(read(nullptr,nullptr,&v)==kHAPError_None)values.push_back(v);}
};
struct BoolCharacteristic : Characteristic {
 template<class F> BoolCharacteristic(uint16_t id,const int *t,F,bool,std::nullptr_t,const char*):Characteristic(id,t){}
};
struct FloatCharacteristic : Characteristic {
 using ReadHandler=std::function<HAPError(HAPAccessoryServerRef*,const HAPFloatCharacteristicReadRequest*,float*)>;
 struct {struct {struct {int units;} float_;} char_;} hap_char_;
 template<class F> FloatCharacteristic(uint16_t id,const int *t,float,float,float,F,bool,std::nullptr_t,const char*):Characteristic(id,t){}
};
struct Service {
 struct {uint16_t iid;} svc_;const int *type;std::string name;bool primary=false;
 std::vector<std::unique_ptr<Characteristic>> chars;
 Service(uint16_t id,const int *t,const char*):svc_{id},type(t){}
 virtual ~Service()=default;
 void AddNameChar(uint16_t,const std::string &n){name=n;}
 void AddChar(Characteristic *c){chars.emplace_back(c);}
 void set_primary(bool value){primary=value;}
};
struct Accessory {
 uint64_t aid;int category;std::string name;std::vector<std::unique_ptr<Service>> services;
 Accessory(uint64_t id,int cat,const std::string &n,int,HAPAccessoryServerRef*):aid(id),category(cat),name(n){}
 void AddHAPService(const int*){}
 void AddService(std::unique_ptr<Service> s){services.push_back(std::move(s));}
 void SetCategory(int cat){category=cat;}
};
}}
struct Address {unsigned char addr[6];};
struct Buffer {const char *p;size_t len;};
struct mgos_bt_gap_scan_result {Address addr;Buffer adv_data;int rssi;Buffer scan_rsp={};};
namespace shelly {
int GetIdentifyCB(){return 0;}
namespace blu {
Sensor sensors[6];Address addresses[6]={};bool known[6]={};
ButtonEvents button_events;ProgrammableButtonEvents programmable_button_events;
std::function<void(uint8_t)> button_event_handler;
bool button_pending=false;double button_press_time=0;uint32_t button_press_sequence=0,packet_sequence[6]={};
void PairingAdvertisement(void*){}
bool PairingBusy(){return pairing;}
void PairingVerified(int){}
bool AcceptSecure(int,const uint8_t*,const uint8_t *data,size_t n,uint8_t *plain,size_t *len){
 if(!authenticated||n<2||n-2>*len)return false;
 memcpy(plain,data+2,n-2);*len=n-2;return true;
}
bool xiaomi=false;
bool SensorPaired(int role){return (role==5&&paired)||(role==3&&xiaomi);}
const char *SensorName(int){return button_name.c_str();}
const Sensor &MotionReading(){return sensors[2];}
const Sensor &ReadingFor(int role){return sensors[role];}
bool SensorReady(int role);
bool HTReady(int role,bool temperature);
bool SecureFor(int,const uint8_t*){return false;}
bool XiaomiFor(int role,const uint8_t*){return role==3&&xiaomi;}
int mgos_sys_config_get_blu_stale_after_s(){return 180;}
bool MotionReady(){return false;}
bool MotionLightReady(){return false;}
'''
for signature in ['bool SensorReady(', 'bool HTReady(', 'void ScanResult(', 'bool ConsumeButtonPress(', 'void SetButtonEventHandler(',
                  'const char *CarName(', 'bool CarPresenceReady(', 'bool CarPresent(']:
    harness += extract(signature) + '\n'
harness += '\n}}\n'
source = (root / 'src/shelly_blu_motion_hap.cpp').read_text()
harness += '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
harness += r'''
using namespace shelly::blu;
void beacon(int packet,int event,bool matching=true){
 uint8_t adv[]={10,0x16,0xd2,0xfc,0x40,0,static_cast<uint8_t>(packet),1,90,0x3a,static_cast<uint8_t>(event)};
 mgos_bt_gap_scan_result r={{{0,0,0,0,0,static_cast<uint8_t>(matching?5:9)}},{reinterpret_cast<const char*>(adv),sizeof(adv)},-50};
 ScanResult(0,&r,nullptr);
}
mgos::hap::UInt8Characteristic *characteristic(mgos::hap::Accessory &a,const int *type){
 for(auto &s:a.services)for(auto &c:s->chars)if(c->type==type)return static_cast<mgos::hap::UInt8Characteristic*>(c.get());
 assert(false);return nullptr;
}
void tick(){for(auto *t:mgos::Timer::timers)t->cb();}
int main(){
 std::unique_ptr<mgos::hap::Accessory> accessory(new mgos::hap::Accessory(1,4,"Garage",0,nullptr));
 paired=true;known[5]=true;addresses[5].addr[5]=5;
 accessory->AddService(CreateCarPresenceService());accessory->AddService(CreateButtonService());
 assert(accessory->services.size()==2&&accessory->aid==1&&accessory->category==4);
 assert(accessory->services[0]->name=="Tesla"&&accessory->services[1]->name=="Remote");
 assert(!accessory->services[0]->primary&&!accessory->services[1]->primary);
 auto *presence=characteristic(*accessory,&kHAPCharacteristicType_OccupancyDetected);
 auto *fault=characteristic(*accessory,&kHAPCharacteristicType_StatusFault);
 auto *event=characteristic(*accessory,&kHAPCharacteristicType_ProgrammableSwitchEvent);
 assert(presence->iid==0x1902&&event->iid==0x1a02);
 uint8_t value=99;assert(event->read(nullptr,nullptr,&value)==kHAPError_InvalidState);
 assert(presence->read(nullptr,nullptr,&value)==kHAPError_None&&value==0);
 tick();beacon(1,0);tick();assert(CarPresent()&&presence->values.back()==1&&event->values.empty()&&!ConsumeButtonPress());
 const int notifications=presence->notifications;tick();assert(presence->notifications==notifications);
 now=55;assert(CarPresent());now=55.01;tick();assert(!CarPresent()&&presence->values.back()==0);
 assert(fault->read(nullptr,nullptr,&value)==kHAPError_None&&value==0); // Absence is a normal reading.
 now=60;beacon(2,0);tick();assert(CarPresent()&&presence->values.back()==1);
 now=60.1;beacon(3,1);assert(event->values==std::vector<int>{0});assert(ConsumeButtonPress()&&!ConsumeButtonPress());
 now=60.3;beacon(3,1);assert(event->values.size()==1&&!ConsumeButtonPress());
 now=60.5;beacon(4,2);assert(event->values.back()==1&&!ConsumeButtonPress());
 now=61;beacon(5,4);assert(event->values.back()==2&&!ConsumeButtonPress());
 now=62;beacon(6,1);assert(ConsumeButtonPress());
 now=62.1;beacon(7,1);assert(!ConsumeButtonPress()); // Keep the local garage debounce.
 assert(event->values==std::vector<int>({0,1,2,0,0})); // Repeated identical events still notify HomeKit.
 beacon(8,0);beacon(9,3);beacon(10,0x80);assert(event->values.size()==5);
 const auto before_packets=packet_sequence[5];
 authenticated=false;beacon(11,1);assert(event->values.size()==5&&packet_sequence[5]==before_packets);
 authenticated=true;beacon(11,1,false);assert(event->values.size()==5);
 now=65;pairing=true;beacon(12,1);pairing=false;beacon(12,1);
 assert(event->values.size()==5&&!ConsumeButtonPress());
 beacon(13,2);assert(event->values.back()==1&&!ConsumeButtonPress());
 bluetooth=false;tick();assert(!CarPresent()&&presence->read(nullptr,nullptr,&value)==kHAPError_None&&value==1);
 assert(fault->read(nullptr,nullptr,&value)==kHAPError_None&&value==1);
 bluetooth=true;tick();assert(presence->values.back()==1);
 scanning=false;tick();assert(!CarPresenceReady()&&presence->read(nullptr,nullptr,&value)==kHAPError_None&&value==1);
 scanning=true;tick();assert(CarPresenceReady()&&presence->values.back()==1);
 accessory.reset();assert(!button_event_handler&&mgos::Timer::timers.empty());
 beacon(14,4); // No callback after the accessory was destroyed.
 car_name="";assert(std::string(CarName())=="Car presence");
 accessory.reset(new mgos::hap::Accessory(1,4,"Garage",0,nullptr));
 now=200;accessory->AddService(CreateCarPresenceService());accessory->AddService(CreateButtonService());
 assert(accessory->services.size()==2&&accessory->aid==1);
 assert(accessory->services[0]->name=="Car presence"&&!CarPresent()); // Missing beacons do not remove accessories.
 // Exercise the same production receive path with a stock Xiaomi advertisement.
 xiaomi=true;known[3]=true;authenticated=false; // No fake authenticated BLE path.
 const uint8_t mac[]={0x58,0x2d,0x34,0x3b,0x2e,0xbf};memcpy(addresses[3].addr,mac,6);
 uint8_t adv[]={25,0x16,0x95,0xfe,0x50,0x20,0xaa,1,0xa3,0xbf,0x2e,0x3b,0x34,0x2d,0x58,0x0d,0x10,4,0xb4,0,0x95,2,0x0a,0x10,1,59};
 mgos_bt_gap_scan_result therm={addresses[3],{reinterpret_cast<const char*>(adv),sizeof(adv)},-55,{}};
 const auto presses=button_press_sequence;now=220;ScanResult(0,&therm,nullptr);
 assert(SensorReady(3)&&sensors[3].temperature10==180&&sensors[3].humidity==66);
 assert(packet_sequence[3]==1&&button_press_sequence==presses&&!ConsumeButtonPress());
 therm.addr.addr[5]^=1;ScanResult(0,&therm,nullptr);assert(packet_sequence[3]==1);
 therm.addr=addresses[3];therm.scan_rsp=therm.adv_data;therm.adv_data={};now=221;
 ScanResult(0,&therm,nullptr);assert(packet_sequence[3]==2); // Scan-response path.
 now=402;assert(!SensorReady(3));
 bluetooth=false;now=222;assert(!SensorReady(3));bluetooth=true;
 xiaomi=false;ScanResult(0,&therm,nullptr);assert(packet_sequence[3]==2); // Explicit binding is required.

}
'''
with tempfile.TemporaryDirectory(prefix='shelly-button-hap-') as temp:
    p=Path(temp);(p/'test.cpp').write_text(harness)
    subprocess.run(['clang++','-std=c++14','-DSHELLY_GARAGE_BLE','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
print('PASS: production BLE path, shared HomeKit accessory, three press types, duplicates, pairing suppression, 45s absence and callback lifetime')
