#!/usr/bin/env python3
"""Exercise the production name/config paths with Frozen and an in-memory config."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
frozen=Path(os.environ.get('SHELLY_MOS_SOURCE',str(Path.home()/'.mos/work/shelly-garage-blu/deps/modules/mongoose-os')))/'src/frozen'
def extract(path,signature):
    s=(root/path).read_text();start=s.index(signature);end=s.index('{',start)+1;depth=1
    while depth:
        depth+=(s[end]=='{')-(s[end]=='}');end+=1
    return s[start:end]
harness=r'''
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <string>
#include "frozen.h"
#define SHELLY_GARAGE_BLE 1
#define STATUS_INVALID_ARGUMENT 1
struct Status {bool ok;static Status OK(){return {true};}};
namespace mgos {
struct Free {void operator()(void *p)const{free(p);}};
using ScopedCPtr=std::unique_ptr<void,Free>;
Status Errorf(int,const char*,...){return {false};}
}
void mgos_conf_set_str(char **p,const char *s){free(*p);*p=strdup(s);}
std::string names[6];
bool enabled=false;int stale=180;std::string closed_mac="AA:BB:CC:DD:EE:01",open_mac="AA:BB:CC:DD:EE:02";
bool mgos_sys_config_get_blu_enable(){return enabled;}
int mgos_sys_config_get_blu_stale_after_s(){return stale;}
const char *mgos_sys_config_get_blu_closed_mac(){return closed_mac.c_str();}
const char *mgos_sys_config_get_blu_open_mac(){return open_mac.c_str();}
void mgos_sys_config_set_blu_enable(bool v){enabled=v;}
void mgos_sys_config_set_blu_stale_after_s(int v){stale=v;}
void mgos_sys_config_set_blu_closed_mac(const char *v){closed_mac=v;}
void mgos_sys_config_set_blu_open_mac(const char *v){open_mac=v;}
struct mgos_config_gdo {
 char *name=strdup("Garage");bool sensor_swap=false,resume_after_stop=false;
 int move_time_ms=20000,pulse_time_ms=500,close_sensor_mode=0,open_sensor_mode=0,out_mode=0;
};
class GarageDoorOpener {
 public:mgos_config_gdo config;mgos_config_gdo *cfg_=&config;
 Status SetConfig(const std::string&,bool*);
 ~GarageDoorOpener(){free(config.name);}
};
'''
for i,role in enumerate(['closed','open','motion','indoor','outdoor','button']):
    harness+=f'const char *mgos_sys_config_get_blu_{role}_name(){{return names[{i}].c_str();}}\n'
    harness+=f'void mgos_sys_config_set_blu_{role}_name(const char *s){{names[{i}]=s;}}\n'
harness+='namespace blu {\nint invalidations=0;void Invalidate(){invalidations++;}\nbool ValidMAC(const char *s){return strlen(s)==17;}\n'
for sig in ['const char *SensorName(', 'void SetSensorName(']:
    harness+=extract('src/shelly_blu_garage.cpp',sig)+'\n'
harness+='}\n'+extract('src/shelly_hap_garage_door_opener.cpp','Status GarageDoorOpener::SetConfig(')
harness+=r'''
int main(){
 GarageDoorOpener g;bool restart=false;
 assert(g.SetConfig(R"({"blu":{"motion_name":"Ruch \"garaż\"","button_name":"Auto","closed_name":"Krańcówka dół","open_name":"Krańcówka góra","indoor_name":"Garaż","outdoor_name":"Ogród"}})",&restart).ok);
 assert(restart&&blu::invalidations==0); // Renaming must not discard live readings/keys.
 assert(names[2]=="Ruch \"garaż\""&&names[5]=="Auto");
 for(int i=0;i<6;i++)assert(std::string(blu::SensorName(i))==names[i]);
 restart=false;assert(g.SetConfig("{}",&restart).ok&&!restart); // Old clients preserve names.
 const std::string old=names[2];
 assert(!g.SetConfig("{\"blu\":{\"closed_name\":\"Changed\",\"motion_name\":\""+std::string(65,'x')+"\"}}",&restart).ok);
 assert(names[2]==old&&names[0]=="Krańcówka dół"&&!restart); // Reject transaction before writes.
 assert(!g.SetConfig(R"({"blu":{"motion_name":"Changed","stale_after_s":1}})",&restart).ok);
 assert(names[2]==old&&!restart);
 assert(g.SetConfig(R"({"blu":{"motion_name":""}})",&restart).ok&&restart);
 assert(std::string(blu::SensorName(2))=="Czujnik ruchu");
 restart=false;
 assert(g.SetConfig("{\"blu\":{\"motion_name\":\""+std::string(64,'x')+"\"}}",&restart).ok&&restart);
 assert(names[2].size()==64&&blu::invalidations==0);
}
'''
with tempfile.TemporaryDirectory() as temp:
    p=Path(temp);(p/'test.cpp').write_text(harness)
    subprocess.run(['clang','-I',str(frozen),'-c',str(frozen/'frozen.c'),'-o',str(p/'frozen.o')],check=True)
    subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror','-I',str(frozen),str(p/'test.cpp'),str(p/'frozen.o'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
print('PASS: all six names, Unicode/quotes, defaults, length bounds, atomic validation and retained telemetry')
