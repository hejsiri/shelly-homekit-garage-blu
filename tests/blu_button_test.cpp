#include <cassert>
#include "shelly_blu_bthome.hpp"
using namespace shelly::blu;
int main(){
 Reading r; ButtonEvents e;
 uint8_t beacon[]={0x40,0,3,1,93,0x3a,0};
 assert(DecodeButton(beacon,sizeof(beacon),&r));assert(r.battery==93);assert(!e.Accept(r,10));
 uint8_t press[]={0x44,0,4,1,92,0x3a,1};
 assert(DecodeButton(press,sizeof(press),&r));assert(e.Accept(r,11));assert(!e.Accept(r,11.1));assert(!e.Accept(r,14));
 press[2]=5;press[6]=2;assert(DecodeButton(press,sizeof(press),&r));assert(!e.Accept(r,16));
 press[2]=6;press[6]=1;assert(DecodeButton(press,sizeof(press),&r));assert(e.Accept(r,17));
 press[2]=7;assert(DecodeButton(press,sizeof(press),&r));assert(!e.Accept(r,17.1));
 uint8_t missing[]={0x40,0x3a,1};assert(DecodeButton(missing,sizeof(missing),&r));assert(!e.Accept(r,20));
 uint8_t malformed[]={0x40,0x3a};assert(!DecodeButton(malformed,sizeof(malformed),&r));
 uint8_t battery[]={0x40,1,101};assert(!DecodeButton(battery,sizeof(battery),&r));
 uint8_t multi[]={0x40,0x3a,1,0x3a,1};assert(!DecodeButton(multi,sizeof(multi),&r));
 uint8_t encrypted[]={0x41,1,99};assert(!DecodeButton(encrypted,sizeof(encrypted),&r));
}
