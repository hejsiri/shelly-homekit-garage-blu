#include <assert.h>
#include <random>
#include <vector>
#include "shelly_blu_bthome.hpp"

using namespace shelly::blu;
int main() {
  // Flags, BTHome UUID, v2 info, packet id, battery, lux, window, button,
  // angle.
  const std::vector<uint8_t> packet = {
      2, 1, 6, 19, 0x16, 0xd2, 0xfc, 0x44, 0,    7, 1, 90,
      5, 1, 2, 3,  0x2d, 0,    0x3a, 1,    0x3f, 0, 0};
  Reading r;
  assert(DecodeAdvertisement(packet.data(), packet.size(), &r));
  assert(!r.window_open && r.battery == 90);
  for (size_t i = 0; i < packet.size(); i++) {
    assert(!DecodeAdvertisement(packet.data(), i, &r));
  }
  auto bad = packet;
  bad[7] |= 1;  // Encryption must not be interpreted as plaintext.
  assert(!DecodeAdvertisement(bad.data(), bad.size(), &r));
  bad = packet;
  bad[7] = 0x20;  // Wrong version.
  assert(!DecodeAdvertisement(bad.data(), bad.size(), &r));
  bad = packet;
  bad[17] = 2;  // Invalid contact value.
  assert(!DecodeAdvertisement(bad.data(), bad.size(), &r));
  bad = packet;
  bad[11] = 101;
  assert(!DecodeAdvertisement(bad.data(), bad.size(), &r));
  bad = packet;
  bad[18] = 0x33;  // Unknown object, no speculative skipping.
  assert(!DecodeAdvertisement(bad.data(), bad.size(), &r));
  const uint8_t no_contact[] = {0x40, 0x00, 1, 0x01, 80};
  assert(!Decode(no_contact, sizeof(no_contact), &r));
  const uint8_t duplicate[] = {0x40, 0x2d, 0, 0x2d, 1};
  assert(!Decode(duplicate, sizeof(duplicate), &r));

  Sensor closed, open;
  assert(!Ready(closed, open, 0, 180));  // Never infer position after reboot.
  closed.Update({false, 80}, 1, -60);
  assert(!Ready(closed, open, 1, 180));
  open.Update({true, 90}, 2, -70);
  assert(Ready(closed, open, 3, 180));
  assert(closed.active && !open.active);
  assert(!Ready(closed, open, 182, 180));  // Either stale sensor blocks.
  closed.Update({true, -1}, 183, -65);
  assert(closed.battery == 80);  // Partial beacon retains battery information.
  open.Update({true, 90}, 183, -70);
  assert(Ready(closed, open, 184, 180));  // Neither endpoint: moving/stopped.
  closed.Update({false, 80}, 185, -60);
  open.Update({false, 90}, 185, -70);
  assert(!Ready(closed, open, 185, 180));  // Contradictory endpoints.
  open.Update({true, 90}, 186, -70);
  assert(Ready(closed, open, 186, 180));  // Recovery.

  // BLU Motion uses 0x21, never the inverted window-contact interpretation.
  const uint8_t motion_on[] = {0x40,0x00,1,0x01,85,0x05,0x10,0x27,0,0x21,1};
  assert(Decode(motion_on, sizeof(motion_on), &r, true));
  assert(r.motion && r.battery == 85 && r.illuminance100 == 10000);
  assert(!Decode(motion_on, sizeof(motion_on), &r));
  Sensor motion_sensor;
  motion_sensor.Update(r, 200, -55, true);
  assert(motion_sensor.active);
  assert(motion_sensor.illuminance100 == 10000 && motion_sensor.IlluminanceFresh(200,180));
  auto motion_off = std::vector<uint8_t>(motion_on, motion_on + sizeof(motion_on));
  motion_off.back() = 0;
  assert(Decode(motion_off.data(), motion_off.size(), &r, true));
  motion_sensor.Update(r, 201, -56, true);
  assert(!motion_sensor.active);
  motion_off.back() = 2;
  assert(!Decode(motion_off.data(), motion_off.size(), &r, true));
  assert(!Decode(motion_on, sizeof(motion_on)-1, &r, true));

  // Light has an independent age: motion-only packets must not refresh it.
  const uint8_t motion_only[] = {0x40,0x21,0};
  assert(Decode(motion_only,sizeof(motion_only),&r,true)&&r.illuminance100==-1);
  motion_sensor.Update(r,400,-55,1);
  assert(motion_sensor.Fresh(400,180)&&!motion_sensor.IlluminanceFresh(400,180));
  assert(motion_sensor.illuminance100==10000);
  const uint8_t dark[] = {0x40,0x05,0,0,0,0x21,0};
  assert(Decode(dark,sizeof(dark),&r,true)&&r.illuminance100==0);
  motion_sensor.Update(r,401,-55,1);assert(motion_sensor.IlluminanceFresh(401,180));
  assert(!motion_sensor.IlluminanceFresh(400,180));
  const uint8_t bright[] = {0x40,0x05,0xff,0xff,0xff,0x21,0};
  assert(Decode(bright,sizeof(bright),&r,true)&&r.illuminance100==16777215);
  for(size_t length=2;length<5;length++)assert(!Decode(bright,length,&r,true));

  const uint8_t ht[] = {0x40,0x00,1,0x01,90,0x2e,65,0x45,0xcc,0xff}; // -5.2 C
  assert(DecodeHT(ht, sizeof(ht), &r));
  assert(r.temperature10 == -52 && r.humidity == 65 && r.battery == 90);
  assert(!Decode(ht, sizeof(ht), &r));
  assert(!DecodeHT(ht, sizeof(ht)-1, &r));
  auto invalid_ht = std::vector<uint8_t>(ht, ht + sizeof(ht));
  invalid_ht[6] = 101;
  assert(!DecodeHT(invalid_ht.data(), invalid_ht.size(), &r));
  const uint8_t incomplete_ht[] = {0x40,0x2e,50};
  assert(!DecodeHT(incomplete_ht, sizeof(incomplete_ht), &r));

  // Exercise arbitrary AD lengths and truncated objects under sanitizers.
  std::mt19937 random(42);
  for (int n = 0; n < 100000; n++) {
    std::vector<uint8_t> bytes(random() % 64);
    for (auto &byte : bytes) byte = random();
    DecodeAdvertisement(bytes.data(), bytes.size(), &r);
    Decode(bytes.data(), bytes.size(), &r);
    Decode(bytes.data(), bytes.size(), &r, true);
    DecodeHT(bytes.data(), bytes.size(), &r);
  }
}
