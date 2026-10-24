#include "shelly_blu_keys.hpp"
#include "shelly_blu_secure.hpp"
#include "nvs.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>
using namespace shelly::blu;
static std::string path;
static std::vector<uint8_t> pending;
static bool fail_commit = false;
static std::string pending_name = "closed";
static std::string RecordPath(const char *name) {
  return std::string(name) == "closed" ? path : path + "." + name;
}
esp_err_t nvs_open(const char *, int, nvs_handle_t *h) { *h = 1; return 0; }
esp_err_t nvs_get_blob(nvs_handle_t, const char *name, void *data, size_t *length) {
  std::ifstream f(RecordPath(name), std::ios::binary);
  if (!f) return 1;
  f.read(static_cast<char *>(data), *length);
  return f.gcount() == static_cast<long>(*length) ? 0 : 1;
}
esp_err_t nvs_set_blob(nvs_handle_t, const char *name, const void *data, size_t n) {
  pending_name = name;
  auto p = static_cast<const uint8_t *>(data); pending.assign(p, p + n); return 0;
}
esp_err_t nvs_commit(nvs_handle_t) {
  if (fail_commit) return 1;
  std::ofstream f(RecordPath(pending_name.c_str()), std::ios::binary); f.write(reinterpret_cast<const char *>(pending.data()), pending.size());
  return f.good() ? 0 : 1;
}
void nvs_close(nvs_handle_t) {}
std::vector<uint8_t> Hex(const char *text) {
  std::vector<uint8_t> result;
  for (size_t i = 0; text[i]; i += 2) { unsigned value; sscanf(text + i, "%2x", &value); result.push_back(value); }
  return result;
}
int main(int argc, char **argv) {
  assert(argc >= 2); path = argv[1];
  // Published BTHome v2 AES-CCM interoperability vector (bthome.io/encryption).
  auto mac = Hex("5448e68f80a5");
  auto key = Hex("231d39c1d7cc1ab1aee224cd096db932");
  auto packet = Hex("d2fc41e445f3c9962b332211006c7c4519");
  uint8_t plain[32] = {}; size_t length = sizeof(plain);
  if (argc == 3) {
    assert(!AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
    assert(XiaomiGATTFor(4,mac.data())&&!XiaomiFor(4,mac.data())&&!SecureFor(4,mac.data()));
    puts("PASS: replay rejected and Xiaomi binding retained after process restart"); return 0;
  }
  SecureRecord record = {}; record.version = 1;
  memcpy(record.mac, mac.data(), 6); memcpy(record.key, key.data(), 16);
  assert(SaveRecord(0, record));
  auto bad = packet; bad.back() ^= 1;
  assert(!AcceptSecure(0, mac.data(), bad.data(), bad.size(), plain, &length));
  bad = packet; bad[3] ^= 1;
  assert(!AcceptSecure(0, mac.data(), bad.data(), bad.size(), plain, &length));
  bad = packet; bad[packet.size() - 8] ^= 1; // Counter is authenticated too.
  assert(!AcceptSecure(0, mac.data(), bad.data(), bad.size(), plain, &length));
  bad = packet; bad[2] &= ~1; // No plaintext downgrade.
  assert(!AcceptSecure(0, mac.data(), bad.data(), bad.size(), plain, &length));
  auto wrong_mac = mac; wrong_mac[0] ^= 1;
  assert(!AcceptSecure(0, wrong_mac.data(), packet.data(), packet.size(), plain, &length));
  for (size_t i = 0; i < packet.size(); i++) {
    assert(!AcceptSecure(0, mac.data(), packet.data(), i, plain, &length));
  }
  // A failure to persist freshness must never release authenticated state.
  fail_commit = true;
  assert(!AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
  fail_commit = false; length = sizeof(plain);
  assert(AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
  auto expected = Hex("4002ca0903bf13");
  assert(length == expected.size() && memcmp(plain, expected.data(), length) == 0);
  length = sizeof(plain);
  assert(!AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
  assert(!NewCounter(true, 100, 99) && !NewCounter(true, 100, 100));
  assert(NewCounter(true, 100, 101));
  assert(!NewCounter(true, UINT32_MAX, 0));
  // Motion has its own persistent key/counter slot; pairing it cannot reset
  // the already accepted garage packet's replay protection.
  assert(SaveRecord(2, record));
  length = sizeof(plain);
  assert(AcceptSecure(2, mac.data(), packet.data(), packet.size(), plain, &length));
  length = sizeof(plain);
  assert(!AcceptSecure(2, mac.data(), packet.data(), packet.size(), plain, &length));
  assert(!AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
    for (int role : {3,4}) {
    assert(SaveRecord(role, record));
    length = sizeof(plain);
    assert(AcceptSecure(role, mac.data(), packet.data(), packet.size(), plain, &length));
    length = sizeof(plain);
    assert(!AcceptSecure(role, mac.data(), packet.data(), packet.size(), plain, &length));
  }
  // A passive thermometer cannot authorize door, motion or button data.
  record.protocol=kProtocolXiaomi;
  for(int slot:{0,1,2,5})assert(!SaveRecord(slot,record));
  assert(SaveRecord(3,record)&&XiaomiFor(3,mac.data())&&!SecureFor(3,mac.data()));
  length=sizeof(plain);
  assert(!AcceptSecure(3,mac.data(),packet.data(),packet.size(),plain,&length));
  assert(!XiaomiFor(3,wrong_mac.data()));
  assert(SaveRecord(4,record));
  record.protocol=kProtocolBTHome;
  assert(SaveRecord(3,record)&&SecureFor(3,mac.data())&&!XiaomiFor(3,mac.data()));
  record.protocol=kProtocolXiaomiGATT;
  for(int slot:{0,1,2,5})assert(!SaveRecord(slot,record));
  assert(SaveRecord(4,record)&&XiaomiGATTFor(4,mac.data())&&!XiaomiFor(4,mac.data())&&!SecureFor(4,mac.data()));
  assert(!XiaomiGATTFor(4,wrong_mac.data()));
  length=sizeof(plain);assert(!AcceptSecure(4,mac.data(),packet.data(),packet.size(),plain,&length));
  // Removed encrypted-Xiaomi protocols cannot create bindings in any role.
  for(uint8_t protocol:{2,3}) for(int slot=0;slot<6;slot++) {
    auto removed=record;removed.protocol=protocol;assert(!SaveRecord(slot,removed));
  }
  assert(!SaveRecord(6, record));
  puts("PASS: independent Motion key and replay counter");
  puts("PASS: real AES-CCM vector, tampering, nonce identity, truncation, plaintext, persistence failure and replay");
}
