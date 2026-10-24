#!/usr/bin/env python3
"""Verify production services through real SDK serialization, routing and reads."""
from pathlib import Path
import json, os, subprocess, sys, tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(os.environ.get('SHELLY_HOMEKIT_ADK_SOURCE',str(Path.home()/'.mos/work/shelly-garage-blu/deps/homekit-adk')))
work=tempfile.TemporaryDirectory(prefix='shelly-hap-database-');p=Path(work.name)
def extract(path, signature):
 source=(root/path).read_text();start=source.index(signature)
 end=source.index('{',start)+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
(p/'debug-database.hpp').write_text(extract('src/shelly_debug.cpp','static void WriteHAPDatabase('))
(p/'select-services.hpp').write_text('''namespace shelly { namespace hap {
struct GarageDoorOpener {
 std::vector<std::unique_ptr<mgos::hap::Service>> blu_sensors_;
 void AddBLUServices(mgos::hap::Accessory *);
};
'''+extract('src/shelly_hap_garage_door_opener.cpp','void GarageDoorOpener::AddBLUServices(')+'\n}}')
(p/'mgos_utils.hpp').write_text('#pragma once\n#define UNUSED_ARG __attribute__((unused))\n')
(p/'mgos.hpp').write_text('''#pragma once
#include <cstdio>
#include <cstring>
#define CS_STRINGIFY_MACRO(x) #x
inline bool mgos_conf_str_empty(const char *s){return !s||!*s;}
inline const char *mgos_sys_config_get_device_sn(){return "test-device";}
inline void mgos_expand_mac_address_placeholders(char*){}
inline const char *mgos_sys_ro_vars_get_fw_version(){return "2.16.1-blu43";}
''')
(p/'mgos_timers.hpp').write_text('''#pragma once
#include <functional>
#include <algorithm>
#include <vector>
#define MGOS_TIMER_REPEAT 1
namespace mgos {
struct Timer;
inline std::vector<Timer*>& timers(){static std::vector<Timer*> values;return values;}
struct Timer {
 explicit Timer(std::function<void()> cb):callback(cb){timers().push_back(this);}
 ~Timer(){auto &v=timers();v.erase(std::remove(v.begin(),v.end(),this),v.end());}
 void Reset(int,int){} std::function<void()> callback;
};
}
''')
(p/'mgos_hap.hpp').write_text('#pragma once\n#include "mgos_hap_accessory.hpp"\nextern const HAPService mgos_hap_accessory_information_service;\n')
(p/'shelly_main.hpp').write_text('#pragma once\nnamespace shelly { inline mgos::hap::Accessory::IdentifyCB GetIdentifyCB(){return nullptr;} }\n')
(p/'motion.cpp').write_text((root/'src/shelly_blu_motion_hap.cpp').read_text())
(p/'mgos_hap.h').write_text('#include \"HAP.h\"\n')
(p/'db.c').write_text((sdk/'src/mgos_homekit_adk_db.c').read_text().split('/* HAP Protocol Information service. */')[0])
(p/'ip-server.c').write_text('#include "' + str(sdk/'HomeKitADK/HAP/HAPIPAccessoryServer.c') + '"\n' + r'''
bool TestFindCharacteristic(HAPAccessoryServerRef *server, uint64_t aid, uint64_t iid,
 const HAPCharacteristic **ch, const HAPService **svc, const HAPAccessory **acc) {
 get_db_ctx(server, aid, iid, ch, svc, acc);
 return *ch != NULL;
}
''')
(p/'test.cpp').write_text(r'''
#include <cassert>
#include "HAP+Internal.h"
#include "HAPIPAccessory.h"
#include "HAPIPCharacteristic.h"
#include <iostream>
#include <cstring>
#include <cstdarg>
#include "mgos_hap.hpp"
#include "shelly_blu_garage.hpp"
#include "shelly_blu_motion_hap.hpp"
#include "mgos_timers.hpp"

std::vector<uint64_t> notifications;
extern "C" void HAPAccessoryServerRaiseEvent(HAPAccessoryServerRef*,const HAPCharacteristic *ch,const HAPService*,const HAPAccessory*){
 notifications.push_back(static_cast<const HAPBaseCharacteristic*>(ch)->iid);
}
namespace shelly { namespace blu {
Sensor sensor;
bool ht=false;
bool SensorPaired(int role){return role==2||role==5||(ht&&role==3);}
const char *SensorName(int role){return role==2?"Motion sensor":role==3?"Indoor":"Button";}
const char *CarName(){return "Car presence";}
bool ready=true;
bool CarPresenceReady(){return ready;}
bool CarPresent(){return true;}
bool SensorReady(int){return true;}
bool HTReady(int,bool temperature){return ready&&sensor.HTFresh(temperature,100,180);}
bool MotionReady(){return ready;}
bool MotionLightReady(){return ready&&sensor.IlluminanceFresh(100,180);}
const Sensor &MotionReading(){return sensor;}
const Sensor &ReadingFor(int){return sensor;}
void SetButtonEventHandler(std::function<void(uint8_t)>){ }
}}
#include "select-services.hpp"

struct mg_connection {std::string text;};
int mg_printf(mg_connection *nc,const char *format,...){
 char buf[512];va_list args;va_start(args,format);int n=vsnprintf(buf,sizeof(buf),format,args);va_end(args);
 assert(n>=0&&n<int(sizeof(buf)));nc->text.append(buf,n);return n;
}
#include "debug-database.hpp"

extern "C" const char *HAPGetVersion(){return "test";}
extern "C" const char *HAPGetBuild(){return "test";}
extern "C" void *HAPAccessoryServerGetClientContext(HAPAccessoryServerRef*){return nullptr;}
extern "C" bool HAPAccessoryServerSupportsService(HAPAccessoryServerRef*,HAPTransportType,const HAPService *s){return memcmp(s->serviceType,&kHAPServiceType_Pairing,sizeof(HAPUUID))!=0;}

extern "C" bool HAPSessionControllerIsAdmin(const HAPSessionRef*){return true;}

extern "C" bool TestFindCharacteristic(HAPAccessoryServerRef*,uint64_t,uint64_t,const HAPCharacteristic**,const HAPService**,const HAPAccessory**);
int main(int argc,char **argv){
 const std::string mode=argc==1?"ready":argv[1];
 shelly::blu::ht=mode.rfind("ht",0)==0;
 shelly::blu::ready=mode!="not-ready";
 auto &sensor=shelly::blu::sensor;sensor.seen=true;sensor.illuminance_last_seen=100;
 sensor.illuminance100=mode=="missing"?-1:mode=="dark"?0:mode=="bright"?16777215:12345;
 if(mode=="stale")sensor.illuminance_last_seen=-100;
 if(shelly::blu::ht){
  sensor.temperature10=-52;sensor.humidity=66;
  sensor.temperature_seen=sensor.humidity_seen=true;
  sensor.temperature_last_seen=mode=="ht-stale"?-100:100;sensor.humidity_last_seen=100;
 }
 std::vector<std::unique_ptr<mgos::hap::Accessory>> accs;
 accs.emplace_back(new mgos::hap::Accessory(1,kHAPAccessoryCategory_GarageDoorOpeners,"Garage",nullptr,nullptr));
 accs[0]->AddHAPService(&mgos_hap_accessory_information_service);
 auto garage=std::unique_ptr<mgos::hap::Service>(new mgos::hap::Service(0x600,&kHAPServiceType_GarageDoorOpener,kHAPServiceDebugDescription_GarageDoorOpener));
 garage->set_primary(true);garage->AddNameChar(0x601,"Garage");
 garage->AddChar(new mgos::hap::UInt8Characteristic(0x602,&kHAPCharacteristicType_CurrentDoorState,0,4,1,[](HAPAccessoryServerRef*,const HAPUInt8CharacteristicReadRequest*,uint8_t *v){*v=1;return kHAPError_None;},true));
 garage->AddChar(new mgos::hap::UInt8Characteristic(0x603,&kHAPCharacteristicType_TargetDoorState,0,1,1,[](HAPAccessoryServerRef*,const HAPUInt8CharacteristicReadRequest*,uint8_t *v){*v=1;return kHAPError_None;},true,[](HAPAccessoryServerRef*,const HAPUInt8CharacteristicWriteRequest*,uint8_t){return kHAPError_None;}));
 garage->AddChar(new mgos::hap::BoolCharacteristic(0x604,&kHAPCharacteristicType_ObstructionDetected,[](HAPAccessoryServerRef*,const HAPBoolCharacteristicReadRequest*,bool *v){*v=false;return kHAPError_None;},true));
 accs[0]->AddService(std::move(garage));
 shelly::hap::GarageDoorOpener owner;owner.AddBLUServices(accs[0].get());
 const HAPAccessory *bridged[]={nullptr};
 HAPAccessoryServer server={};server.primaryAccessory=accs[0]->GetHAPAccessory();server.ip.bridgedAccessories=bridged;
 accs[0]->set_server(reinterpret_cast<HAPAccessoryServerRef*>(&server));
 auto tick=[](){for(auto *timer:mgos::timers())timer->callback();};tick();
 if(mode=="ready"){
  const auto before=std::count(notifications.begin(),notifications.end(),0x1b02);
  sensor.illuminance100++;tick();assert(std::count(notifications.begin(),notifications.end(),0x1b02)==before+1);
  tick();assert(std::count(notifications.begin(),notifications.end(),0x1b02)==before+1);
  sensor.illuminance100--;tick();
 }
 if(mode=="lost"){shelly::blu::ready=false;sensor.illuminance100=99999;tick();}
 mg_connection diagnostics;WriteHAPDatabase(&diagnostics,server.primaryAccessory);
 for (const char *expected : {"AID 1 service 6144 type 85 name Motion sensor", "AID 1 service 6400 type 86 name Car presence", "AID 1 service 6656 type 89 name Button", "characteristic 6146 type 22", "characteristic 6402 type 71", "characteristic 6658 type 73", "AID 1 service 6912 type 84 name Light sensor", "characteristic 6914 type 6B"})
  assert(diagnostics.text.find(expected)!=std::string::npos);
 HAPIPSessionDescriptorRef session={};auto *descriptor=reinterpret_cast<HAPIPSessionDescriptor*>(&session);descriptor->server=reinterpret_cast<HAPAccessoryServerRef*>(&server);HAPIPAccessorySerializationContext ctx;HAPIPAccessoryCreateSerializationContext(&ctx);
 std::vector<uint64_t> reads={0x1802,0x1902,0x1a02,0x1b02};
 if(shelly::blu::ht){reads.push_back(0x1502);reads.push_back(0x1512);}
 for (uint64_t iid : reads) {
  const HAPCharacteristic *ch=nullptr;const HAPService *svc=nullptr;const HAPAccessory *acc=nullptr;
  assert(TestFindCharacteristic(reinterpret_cast<HAPAccessoryServerRef*>(&server),1,iid,&ch,&svc,&acc));
  assert(svc->iid==(iid-2)&&acc->aid==1);
  HAPIPSessionReadResult result={};char bytes[128]={};HAPIPByteBuffer buf={};buf.data=bytes;buf.capacity=sizeof(bytes);buf.limit=sizeof(bytes);
  HAPIPSessionHandleReadRequest(&session,kHAPIPSessionContext_GetAccessories,ch,svc,acc,&result,&buf);
  assert(result.status==0);
 }
 for (uint64_t retired : {0x1402,0x1602,0x1702}) {
  const HAPCharacteristic *ch=nullptr;const HAPService *svc=nullptr;const HAPAccessory *acc=nullptr;
  assert(!TestFindCharacteristic(reinterpret_cast<HAPAccessoryServerRef*>(&server),1,retired,&ch,&svc,&acc));
 }
 std::string json;
 while(!HAPIPAccessorySerializationIsComplete(&ctx)){char data[1536]={};size_t n=0;
  auto err=HAPIPAccessorySerializeReadResponse(&ctx,reinterpret_cast<HAPAccessoryServerRef*>(&server),&session,data,sizeof(data)/2,sizeof(data),&n);
  assert(err==kHAPError_None);json.append(data,n);
 }
 std::cout<<json;
}

''')
flags=['-DHAP_IP=1','-DHAP_BLE=0','-DSHELLY_GARAGE_BLE','-DHAP_DISABLE_ASSERTS=1','-DHAP_DISABLE_PRECONDITIONS=1','-DHAP_LOG_LEVEL=0','-ffunction-sections','-fdata-sections','-Wno-nullability-completeness','-Wno-unknown-pragmas']
for inc in [p,root/'src',sdk/'include',sdk/'HomeKitADK/HAP',sdk/'HomeKitADK/PAL',sdk/'HomeKitADK/External/HTTP',sdk/'HomeKitADK/External/JSON',sdk/'HomeKitADK/External/Base64']:
 flags+=['-I',str(inc)]
objs=[]
for f in [sdk/'HomeKitADK/HAP/HAPCharacteristicTypes.c',sdk/'HomeKitADK/HAP/HAPServiceTypes.c',
 p/'ip-server.c',sdk/'HomeKitADK/HAP/HAPCharacteristic.c',sdk/'HomeKitADK/HAP/HAPTLV.c',sdk/'HomeKitADK/HAP/HAPTLVWriter.c',sdk/'HomeKitADK/External/Base64/util_base64.c',sdk/'HomeKitADK/HAP/HAPIPAccessory.c',sdk/'HomeKitADK/HAP/HAPIPCharacteristic.c',sdk/'HomeKitADK/HAP/HAPUUID.c',
 sdk/'HomeKitADK/HAP/HAPJSONUtils.c',sdk/'HomeKitADK/PAL/HAPAssert.c',sdk/'HomeKitADK/PAL/HAPBase+Crypto.c',sdk/'HomeKitADK/PAL/HAPLog.c',sdk/'HomeKitADK/PAL/Mock/HAPPlatformAbort.c',sdk/'HomeKitADK/PAL/Mock/HAPPlatformLog.c',sdk/'HomeKitADK/HAP/HAPRequestHandlers+AccessoryInformation.c',
 *[sdk/'HomeKitADK/PAL'/('HAPBase+'+name+'.c') for name in ['RawBuffer','String','Int','Float','Double','UTF8']],p/'db.c']:
 out=p/(f.stem+'.o');subprocess.run(['clang',*flags,'-c',str(f),'-o',str(out)],check=True);objs.append(str(out))
subprocess.run(['clang++','-std=c++14',*flags,str(p/'test.cpp'),str(p/'motion.cpp'),str(sdk/'src/mgos_hap_chars.cpp'),str(sdk/'src/mgos_hap_service.cpp'),str(sdk/'src/mgos_hap_accessory.cpp'),*objs,('-Wl,-dead_strip' if sys.platform=='darwin' else '-Wl,--gc-sections'),'-o',str(p/'test')],check=True)
for mode in ['ready','not-ready','missing','dark','bright','stale','lost','ht','ht-stale']:
 ready=mode not in ['not-ready','lost']
 args=[str(p/'test')]+([] if mode=='ready' else [mode])
 db=json.loads(subprocess.check_output(args))
 assert len(db['accessories'])==1
 accessory=db['accessories'][0];assert accessory['aid']==1
 services={v['type']:v for v in accessory['services']}
 assert set(services)==({'3E','41','85','86','89','84','8A','82'} if mode.startswith('ht') else {'3E','41','85','86','89','84'})
 assert [v['type'] for v in services.values() if v['primary']]==['41']
 assert services['85']['iid']==0x1800 and services['86']['iid']==0x1900 and services['89']['iid']==0x1a00
 ids=[]
 for service in services.values():
  ids.append(service['iid']);ids.extend(c['iid'] for c in service['characteristics'])
  assert not service['hidden']
 assert len(ids)==len(set(ids))
 # Retire identities seen under conflicting service types in Apple Home logs.
 assert not set(ids).intersection(range(0x1400,0x1405))
 assert not set(ids).intersection(range(0x1600,0x1605))
 assert not set(ids).intersection(range(0x1700,0x1703))
 assert {c['type'] for c in services['85']['characteristics']}=={'23','22','77','75'}
 assert {c['type'] for c in services['86']['characteristics']}=={'23','71','77','75'}
 assert {c['type'] for c in services['89']['characteristics']}=={'23','73'}
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
 assert services['84']['iid']==0x1b00
 light={c['type']:c for c in services['84']['characteristics']}
 expected=100000 if mode=='bright' else 123.45 if mode in ['ready','lost','ht','ht-stale'] else 0.0001
 assert abs(light['6B']['value']-expected)<0.00001,(mode,light)
 assert light['6B']['format']=='float' and light['6B']['unit']=='lux'
 assert light['6B']['minValue']==0.0001 and light['6B']['maxValue']==100000
 assert set(light['6B']['perms'])=={'pr','ev'}
 light_ready=ready and mode not in ['missing','stale']
 assert light['77']['value']==(0 if light_ready else 1)
 assert light['75']['value']==(1 if light_ready else 0)
 if mode.startswith('ht'):
  temp={c['type']:c for c in services['8A']['characteristics']}
  hum={c['type']:c for c in services['82']['characteristics']}
  assert services['8A']['iid']==0x1500 and services['82']['iid']==0x1510
  assert temp['11']['value']==(-5.2 if mode=='ht' else 0)
  assert temp['77']['value']==(0 if mode=='ht' else 1)
  assert hum['10']['value']==66 and hum['77']['value']==0
  assert temp['11']['format']=='float' and hum['10']['format']=='float'

work.cleanup()
print('PASS: real Apple SDK routing, reads and serialization; stable services, light units/range, missing/stale readings, notifications, receiver loss and event semantics')
