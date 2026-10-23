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

  // Exercise arbitrary AD lengths and truncated objects under sanitizers.
  std::mt19937 random(42);
  for (int n = 0; n < 100000; n++) {
    std::vector<uint8_t> bytes(random() % 64);
    for (auto &byte : bytes) byte = random();
    DecodeAdvertisement(bytes.data(), bytes.size(), &r);
    Decode(bytes.data(), bytes.size(), &r);
  }
}
