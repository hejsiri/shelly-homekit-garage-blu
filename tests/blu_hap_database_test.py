#!/usr/bin/env python3
"""Verify real SDK /accessories serialization with mocked transport and device state."""
from pathlib import Path
import json, os, subprocess, sys, tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(os.environ.get('SHELLY_HOMEKIT_ADK_SOURCE',str(Path.home()/'.mos/work/shelly-garage-blu/deps/homekit-adk')))
work=tempfile.TemporaryDirectory(prefix='shelly-hap-database-');p=Path(work.name)
(p/'mgos_utils.hpp').write_text('#pragma once\n#define UNUSED_ARG __attribute__((unused))\n')
(p/'mgos.hpp').write_text('''#pragma once
#include <cstdio>
#include <cstring>
#define CS_STRINGIFY_MACRO(x) #x
inline bool mgos_conf_str_empty(const char *s){return !s||!*s;}
inline const char *mgos_sys_config_get_device_sn(){return "test-device";}
inline void mgos_expand_mac_address_placeholders(char*){}
inline const char *mgos_sys_ro_vars_get_fw_version(){return "2.16.1-blu41";}
''')
(p/'mgos_timers.hpp').write_text('''#pragma once
#include <functional>
#define MGOS_TIMER_REPEAT 1
namespace mgos { struct Timer { explicit Timer(std::function<void()> cb):callback(cb){} void Reset(int,int){} std::function<void()> callback; }; }
''')
(p/'mgos_hap.hpp').write_text('#pragma once\n#include "mgos_hap_accessory.hpp"\nextern const HAPService mgos_hap_accessory_information_service;\n')
(p/'shelly_main.hpp').write_text('#pragma once\nnamespace shelly { inline mgos::hap::Accessory::IdentifyCB GetIdentifyCB(){return nullptr;} }\n')
(p/'motion.cpp').write_text((root/'src/shelly_blu_motion_hap.cpp').read_text())
(p/'mgos_hap.h').write_text('#include \"HAP.h\"\n')
(p/'db.c').write_text((sdk/'src/mgos_homekit_adk_db.c').read_text().split('/* HAP Protocol Information service. */')[0])
(p/'test.cpp').write_text(r'''
#include <cassert>
#include "HAP+Internal.h"
#include "HAPIPAccessory.h"
#include "HAPIPCharacteristic.h"
#include <iostream>
#include <cstring>
#include "mgos_hap.hpp"
#include "shelly_blu_garage.hpp"
#include "shelly_blu_motion_hap.hpp"

extern "C" void HAPAccessoryServerRaiseEvent(HAPAccessoryServerRef*,const HAPCharacteristic*,const HAPService*,const HAPAccessory*){}
namespace shelly { namespace blu {
Sensor sensor;
bool SensorPaired(int role){return role==5;}
const char *SensorName(int role){return role==2?"Motion sensor":"Button";}
const char *CarName(){return "Car presence";}
bool ready=true;
bool CarPresenceReady(){return ready;}
bool CarPresent(){return true;}
bool SensorReady(int){return true;}
bool MotionReady(){return ready;}
const Sensor &MotionReading(){return sensor;}
const Sensor &ReadingFor(int){return sensor;}
void SetButtonEventHandler(std::function<void(uint8_t)>){ }
}}

extern "C" const char *HAPGetVersion(){return "test";}
extern "C" const char *HAPGetBuild(){return "test";}
extern "C" void *HAPAccessoryServerGetClientContext(HAPAccessoryServerRef*){return nullptr;}
extern "C" bool HAPAccessoryServerSupportsService(HAPAccessoryServerRef*,HAPTransportType,const HAPService *s){return memcmp(s->serviceType,&kHAPServiceType_Pairing,sizeof(HAPUUID))!=0;}
extern "C" bool HAPIPSessionAreEventNotificationsEnabled(HAPIPSessionDescriptorRef*,const HAPCharacteristic*,const HAPService*,const HAPAccessory*){return false;}
extern "C" void HAPIPSessionHandleReadRequest(HAPIPSessionDescriptorRef*,HAPIPSessionContext,const HAPCharacteristic *ch,const HAPService *s,const HAPAccessory *a,HAPIPSessionReadResult *result,HAPIPByteBuffer *buf){
 auto *base=static_cast<const HAPBaseCharacteristic*>(ch);result->status=0;HAPError err=kHAPError_None;HAPAccessoryServerRef server={};
 switch(base->format){
  case kHAPCharacteristicFormat_Bool:{auto *c=static_cast<const HAPBoolCharacteristic*>(ch);HAPBoolCharacteristicReadRequest req={};req.characteristic=c;req.service=s;req.accessory=a;bool v=false;err=c->callbacks.handleRead(&server,&req,&v,nullptr);result->value.unsignedIntValue=v;break;}
  case kHAPCharacteristicFormat_UInt8:{auto *c=static_cast<const HAPUInt8Characteristic*>(ch);HAPUInt8CharacteristicReadRequest req={};req.characteristic=c;req.service=s;req.accessory=a;uint8_t v=0;err=c->callbacks.handleRead(&server,&req,&v,nullptr);result->value.unsignedIntValue=v;break;}
  case kHAPCharacteristicFormat_String:{auto *c=static_cast<const HAPStringCharacteristic*>(ch);HAPStringCharacteristicReadRequest req={};req.characteristic=c;req.service=s;req.accessory=a;err=c->callbacks.handleRead(&server,&req,buf->data,buf->capacity,nullptr);result->value.stringValue.bytes=buf->data;result->value.stringValue.numBytes=strlen(buf->data);break;}
  default:result->status=-70402;
 }
 if(err)result->status=-70402;
}
int main(int argc,char**){
 shelly::blu::ready=argc==1;
 std::vector<std::unique_ptr<mgos::hap::Accessory>> accs;
 accs.emplace_back(new mgos::hap::Accessory(1,kHAPAccessoryCategory_GarageDoorOpeners,"Garage",nullptr,nullptr));
 accs[0]->AddHAPService(&mgos_hap_accessory_information_service);
 auto garage=std::unique_ptr<mgos::hap::Service>(new mgos::hap::Service(0x600,&kHAPServiceType_GarageDoorOpener,kHAPServiceDebugDescription_GarageDoorOpener));
 garage->set_primary(true);garage->AddNameChar(0x601,"Garage");
 garage->AddChar(new mgos::hap::UInt8Characteristic(0x602,&kHAPCharacteristicType_CurrentDoorState,0,4,1,[](HAPAccessoryServerRef*,const HAPUInt8CharacteristicReadRequest*,uint8_t *v){*v=1;return kHAPError_None;},true));
 garage->AddChar(new mgos::hap::UInt8Characteristic(0x603,&kHAPCharacteristicType_TargetDoorState,0,1,1,[](HAPAccessoryServerRef*,const HAPUInt8CharacteristicReadRequest*,uint8_t *v){*v=1;return kHAPError_None;},true,[](HAPAccessoryServerRef*,const HAPUInt8CharacteristicWriteRequest*,uint8_t){return kHAPError_None;}));
 garage->AddChar(new mgos::hap::BoolCharacteristic(0x604,&kHAPCharacteristicType_ObstructionDetected,[](HAPAccessoryServerRef*,const HAPBoolCharacteristicReadRequest*,bool *v){*v=false;return kHAPError_None;},true));
 accs[0]->AddService(std::move(garage));
 accs[0]->AddService(shelly::blu::CreateMotionService());
 accs[0]->AddService(shelly::blu::CreateCarPresenceService());
 accs[0]->AddService(shelly::blu::CreateButtonService());
 const HAPAccessory *bridged[]={nullptr};
 HAPAccessoryServer server={};server.primaryAccessory=accs[0]->GetHAPAccessory();server.ip.bridgedAccessories=bridged;
 HAPIPSessionDescriptorRef session={};HAPIPAccessorySerializationContext ctx;HAPIPAccessoryCreateSerializationContext(&ctx);
 std::string json;
 while(!HAPIPAccessorySerializationIsComplete(&ctx)){char data[1536]={};size_t n=0;
  auto err=HAPIPAccessorySerializeReadResponse(&ctx,reinterpret_cast<HAPAccessoryServerRef*>(&server),&session,data,1,sizeof(data),&n);
  assert(err==kHAPError_None);json.append(data,n);
 }
 std::cout<<json;
}

''')
flags=['-DHAP_IP=1','-DHAP_BLE=0','-DSHELLY_GARAGE_BLE','-DHAP_DISABLE_ASSERTS=1','-DHAP_DISABLE_PRECONDITIONS=1','-DHAP_LOG_LEVEL=0','-ffunction-sections','-fdata-sections','-Wno-nullability-completeness','-Wno-unknown-pragmas']
for inc in [p,root/'src',sdk/'include',sdk/'HomeKitADK/HAP',sdk/'HomeKitADK/PAL',sdk/'HomeKitADK/External/HTTP',sdk/'HomeKitADK/External/JSON']:
 flags+=['-I',str(inc)]
objs=[]
for f in [sdk/'HomeKitADK/HAP/HAPCharacteristicTypes.c',sdk/'HomeKitADK/HAP/HAPServiceTypes.c',
 sdk/'HomeKitADK/HAP/HAPIPAccessory.c',sdk/'HomeKitADK/HAP/HAPIPCharacteristic.c',sdk/'HomeKitADK/HAP/HAPUUID.c',
 sdk/'HomeKitADK/HAP/HAPJSONUtils.c',sdk/'HomeKitADK/PAL/HAPAssert.c',sdk/'HomeKitADK/PAL/HAPBase+Crypto.c',sdk/'HomeKitADK/PAL/HAPLog.c',sdk/'HomeKitADK/PAL/Mock/HAPPlatformAbort.c',sdk/'HomeKitADK/PAL/Mock/HAPPlatformLog.c',sdk/'HomeKitADK/HAP/HAPRequestHandlers+AccessoryInformation.c',
 *[sdk/'HomeKitADK/PAL'/('HAPBase+'+name+'.c') for name in ['RawBuffer','String','Int','Float','Double','UTF8']],p/'db.c']:
 out=p/(f.stem+'.o');subprocess.run(['clang',*flags,'-c',str(f),'-o',str(out)],check=True);objs.append(str(out))
subprocess.run(['clang++','-std=c++14',*flags,str(p/'test.cpp'),str(p/'motion.cpp'),str(sdk/'src/mgos_hap_chars.cpp'),str(sdk/'src/mgos_hap_service.cpp'),str(sdk/'src/mgos_hap_accessory.cpp'),*objs,('-Wl,-dead_strip' if sys.platform=='darwin' else '-Wl,--gc-sections'),'-o',str(p/'test')],check=True)
for ready in [True, False]:
 args=[str(p/'test')]+([] if ready else ['not-ready'])
 db=json.loads(subprocess.check_output(args))
 assert len(db['accessories'])==1
 accessory=db['accessories'][0];assert accessory['aid']==1
 services={v['type']:v for v in accessory['services']}
 assert set(services)=={'3E','41','85','86','89'}
 assert [v['type'] for v in services.values() if v['primary']]==['41']
 assert services['85']['iid']==0x1400 and services['86']['iid']==0x1600 and services['89']['iid']==0x1700
 ids=[]
 for service in services.values():
  ids.append(service['iid']);ids.extend(c['iid'] for c in service['characteristics'])
  assert not service['hidden']
 assert len(ids)==len(set(ids))
 for service_type,value_type in [('85','22'),('86','71')]:
  chars={c['type']:c for c in services[service_type]['characteristics']}
  assert chars[value_type]['value'] in [0,1] and chars[value_type]['value'] is not None
  assert chars['77']['value']==(0 if ready else 1)
  assert chars['75']['value']==(1 if ready else 0)
  assert set(chars[value_type]['perms'])=={'pr','ev'}
 chars={c['type']:c for c in services['89']['characteristics']}
 assert chars['73']['value'] is None and chars['73']['maxValue']==2
 assert chars['23']['value']=='Button'
 assert next(c['value'] for c in services['86']['characteristics'] if c['type']=='23')=='Car presence'
work.cleanup()
print('PASS: real HAP wrappers and Apple /accessories serialization, one garage accessory, unique IDs, occupancy/motion typed values, faults, active state and null switch events')
