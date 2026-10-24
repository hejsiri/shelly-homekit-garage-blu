#include <cassert>
#include <random>
#include <vector>
#include "shelly_blu_xiaomi.hpp"
using namespace shelly::blu;
int main() {
  const uint8_t mac[] = {0x58,0x2d,0x34,0x3b,0x2e,0xbf};
  // Real stock LYWSDCGQ vector from Bluetooth-Devices/xiaomi-ble's
  // test_Xiaomi_LYWSDCGQ: 18.0 C, 66.1 % RH, 59 % battery.
  const std::vector<uint8_t> packet = {
      0x50,0x20,0xaa,1,0xa3,0xbf,0x2e,0x3b,0x34,0x2d,0x58,
      0x0d,0x10,4,0xb4,0,0x95,2,0x0a,0x10,1,59};
  Reading r;
  assert(DecodeXiaomi(packet.data(),packet.size(),mac,&r));
  assert(r.temperature_present && r.temperature10==180 && r.humidity==66 && r.battery==59);
  Sensor sensor;
  sensor.Update(r,1,-55,2);
  assert(sensor.HTFresh(true,1,180)&&sensor.HTFresh(false,1,180));
  // Partial objects retain the other measurement and its own age.
  auto partial=std::vector<uint8_t>(packet.begin(),packet.begin()+11);
  partial.insert(partial.end(),{0x04,0x10,2,0xcc,0xff});
  assert(DecodeXiaomi(partial.data(),partial.size(),mac,&r));
  assert(r.temperature10==-52 && r.humidity==-1);
  sensor.Update(r,200,-53,2);
  assert(sensor.temperature10==-52 && sensor.humidity==66);
  assert(sensor.HTFresh(true,200,180)&&!sensor.HTFresh(false,200,180));
  partial.resize(11);partial.insert(partial.end(),{0x06,0x10,2,0x35,2});
  assert(DecodeXiaomi(partial.data(),partial.size(),mac,&r));
  assert(!r.temperature_present && r.humidity==57);
  sensor.Update(r,201,-53,2);
  assert(sensor.temperature10==-52 && sensor.HTFresh(false,201,180));
  partial.resize(11);partial.insert(partial.end(),{0x0a,0x10,1,80});
  assert(DecodeXiaomi(partial.data(),partial.size(),mac,&r));
  sensor.Update(r,400,-53,2);
  assert(sensor.battery==80 && sensor.Fresh(400,180));
  assert(!sensor.HTFresh(true,400,180)&&!sensor.HTFresh(false,400,180));
  // A valid prefix ending on an object boundary is a valid partial reading.
  for(size_t n=0;n<packet.size();n++)
    assert(DecodeXiaomi(packet.data(),n,mac,&r)==(n==18));
  auto bad=packet;bad[0]|=8;assert(!DecodeXiaomi(bad.data(),bad.size(),mac,&r));
  bad=packet;bad[0]|=0x80;assert(!DecodeXiaomi(bad.data(),bad.size(),mac,&r));
  bad=packet;bad[2]=0x5b;assert(!DecodeXiaomi(bad.data(),bad.size(),mac,&r));
  bad=packet;bad[5]^=1;assert(!DecodeXiaomi(bad.data(),bad.size(),mac,&r));
  bad=packet;bad.back()=101;assert(!DecodeXiaomi(bad.data(),bad.size(),mac,&r));
  bad=packet;bad[17]=0xff;assert(!DecodeXiaomi(bad.data(),bad.size(),mac,&r));
  bad=packet;bad[13]=5;assert(!DecodeXiaomi(bad.data(),bad.size(),mac,&r));
  bad=packet;bad.push_back(1);assert(!DecodeXiaomi(bad.data(),bad.size(),mac,&r));
  bad=packet;bad.insert(bad.end(),{0x0a,0x10,1,80});assert(!DecodeXiaomi(bad.data(),bad.size(),mac,&r));
  auto capability=packet;capability[0]|=0x20;capability.insert(capability.begin()+11,{0x20,0});
  assert(DecodeXiaomi(capability.data(),capability.size(),mac,&r));
  auto no_mac=packet;no_mac[0]&=~0x10;no_mac.erase(no_mac.begin()+5,no_mac.begin()+11);
  assert(DecodeXiaomi(no_mac.data(),no_mac.size(),mac,&r));
  std::vector<uint8_t> adv={2,1,6,uint8_t(packet.size()+3),0x16,0x95,0xfe};
  adv.insert(adv.end(),packet.begin(),packet.end());
  assert(DecodeXiaomiAdvertisement(adv.data(),adv.size(),mac,&r));
  for(size_t n=0;n<adv.size();n++)assert(!DecodeXiaomiAdvertisement(adv.data(),n,mac,&r));
  auto duplicate=adv;duplicate.insert(duplicate.end(),adv.begin()+3,adv.end());
  assert(!DecodeXiaomiAdvertisement(duplicate.data(),duplicate.size(),mac,&r));
  uint8_t key[16];
  assert(ParseXiaomiBindkey("e9ea895fac7cca6d30532432a516f3a8",key)&&key[0]==0xe9);
  assert(ParseXiaomiBindkey("E9EA895FAC7CCA6D30532432A516F3A8",key));
  for(const char *invalid:{"", "123456", "e9ea895fac7cca6d30532432a516f3ag", " e9ea895fac7cca6d30532432a516f3a8"})
    assert(!ParseXiaomiBindkey(invalid,key));
  std::mt19937 random(73);
  for(int n=0;n<100000;n++) {
    std::vector<uint8_t> bytes(random()%80);
    for(auto &v:bytes)v=random();
    DecodeXiaomiObjects(bytes.data(),bytes.size(),&r,0x098b);
    DecodeXiaomi(bytes.data(),bytes.size(),mac,&r);
    DecodeXiaomiAdvertisement(bytes.data(),bytes.size(),mac,&r);
  }
}
