#include "shelly_blu_mibeacon.hpp"
#include <cassert>
#include <vector>
#include <cstdio>
using namespace shelly::blu;
int main(){
 const uint8_t mac[]={0x58,0x2d,0x34,0x40,0x3a,0x4d};
 const uint8_t door[]={0xc8,4,0x4d,0x3a,0x40,0x34,0x2d,0x58,4,1,1,0x0f,1,0xef};
 Reading r;bool state;uint16_t model;
 assert(DecodeQingping(door,sizeof(door),mac,&model,&r,&state)&&model==0x03d6&&state&&!r.window_open);
 for(size_t n=0;n<11;n++)assert(!DecodeQingping(door,n,mac,&model,&r,&state));
 auto bad=std::vector<uint8_t>(door,door+sizeof(door));bad[2]^=1;
 assert(!DecodeQingping(bad.data(),bad.size(),mac,&model,&r,&state));
 bad.assign(door,door+sizeof(door));bad[10]=0;
 assert(DecodeQingping(bad.data(),bad.size(),mac,&model,&r,&state)&&state&&r.window_open);
 bad[10]=3;assert(!DecodeQingping(bad.data(),bad.size(),mac,&model,&r,&state));
 const uint8_t tm[]={8,0x16,0x4d,0x3a,0x40,0x34,0x2d,0x58,1,4,0xd8,0,0xbb,1,2,1,100};
 assert(DecodeQingping(tm,sizeof(tm),mac,&model,&r,&state)&&model==0x0347&&r.temperature10==216&&r.humidity==44&&r.battery==100&&!state);
 const uint8_t motion[]={8,0x12,0x4d,0x3a,0x40,0x34,0x2d,0x58,8,4,1,100,0,0};
 assert(DecodeQingping(motion,sizeof(motion),mac,&model,&r,&state)&&model==0x0a83&&state&&r.motion&&r.illuminance100==10000);
 bad.assign(motion,motion+sizeof(motion));bad[0]=0x48;
 assert(DecodeQingping(bad.data(),bad.size(),mac,&model,&r,&state)&&r.motion&&state&&r.illuminance100==-1);
 bad[10]=0;assert(DecodeQingping(bad.data(),bad.size(),mac,&model,&r,&state)&&!r.motion&&state);
 const uint8_t mbdoor[]={0x50,0x40,0xd6,3,1,0x4d,0x3a,0x40,0x34,0x2d,0x58,0x19,0x10,1,0};
 assert(DecodeQingping(mbdoor,sizeof(mbdoor),mac,&model,&r,&state)&&model==0x03d6&&state&&r.window_open);
 const uint8_t lux[]={0x0f,0,3,100,0,0},clear[]={0x17,0x10,4,60,0,0,0},recent[]={0x17,0x10,4,30,0,0,0};
 assert(MiObjects(0x0a83,lux,sizeof(lux),&r,&state)&&state&&r.motion&&r.illuminance100==10000);
 assert(MiObjects(0x0a83,clear,sizeof(clear),&r,&state)&&state&&!r.motion);
 assert(MiObjects(0x0a83,recent,sizeof(recent),&r,&state)&&state&&r.motion);
 const uint8_t bat[]={0x0a,0x10,1,88};
 assert(MiObjects(0x0a83,bat,sizeof(bat),&r,&state)&&!state&&r.battery==88);
 // Native battery-only packets must not be mistaken for motion-clear events.
 const uint8_t native_battery[]={8,0x12,0x4d,0x3a,0x40,0x34,0x2d,0x58,2,1,73};
 assert(DecodeQingping(native_battery,sizeof(native_battery),mac,&model,&r,&state)&&!state&&r.battery==73);
 const uint8_t mi_battery[]={0x50,0x50,0x83,0x0a,2,0x4d,0x3a,0x40,0x34,0x2d,0x58,0x0a,0x10,1,64};
 assert(DecodeQingping(mi_battery,sizeof(mi_battery),mac,&model,&r,&state)&&!state&&r.battery==64);
 const uint8_t alternate[]={0x03,0x48,1,52};
 assert(MiObjects(0x0a83,alternate,sizeof(alternate),&r,&state)&&!state&&r.battery==52);
 Sensor sensor; sensor.battery=r.battery;
 assert(MiObjects(0x0a83,clear,sizeof(clear),&r,&state));
 sensor.Update(r,10,-60,1);
 assert(sensor.battery==52&&!sensor.active);
 const uint8_t invalid_battery[]={0x03,0x48,1,101};
 assert(!MiObjects(0x0a83,invalid_battery,sizeof(invalid_battery),&r,&state));
 assert(!MiRoleModel(2,0x03d6)&&MiRoleModel(2,0x0a83)&&MiRoleModel(3,0x0b48)&&!MiRoleModel(0,0x0347));
 for(uint16_t id : {uint16_t(0x3531),uint16_t(0x4c60)}) {
   assert(MiRoleModel(2,id)&&!MiRoleModel(0,id)&&!MiRoleModel(3,id));
   const uint8_t event[]={0x08,0x4a,4,0,0,0x4c,0x42}; // float 51 lx
   assert(MiObjects(id,event,sizeof(event),&r,&state)&&state&&r.motion&&r.illuminance100==5100);
   const uint8_t clear2[]={0x18,0x48,2,31,0},recent2[]={0x18,0x48,2,30,0};
   assert(MiObjects(id,clear2,sizeof(clear2),&r,&state)&&state&&!r.motion);
   assert(MiObjects(id,recent2,sizeof(recent2),&r,&state)&&state&&r.motion);
   assert(MiObjects(id,alternate,sizeof(alternate),&r,&state)&&!state&&r.battery==52);
   const uint8_t nan[]={0x08,0x4a,4,0,0,0xc0,0x7f},negative[]={0x08,0x4a,4,0,0,0x80,0xbf};
   assert(!MiObjects(id,nan,sizeof(nan),&r,&state));
   assert(!MiObjects(id,negative,sizeof(negative),&r,&state));
   const uint8_t short_float[]={0x08,0x4a,3,0,0,0};
   assert(!MiObjects(id,short_float,sizeof(short_float),&r,&state));
 }
 puts("PASS: Qingping native and MiBeacon models, contact polarity, motion/clear, event light suppression, field validation and MAC identity");
}
