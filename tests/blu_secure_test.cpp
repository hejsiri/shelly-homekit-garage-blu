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
esp_err_t nvs_open(const char *, int, nvs_handle_t *h) { *h = 1; return 0; }
esp_err_t nvs_get_blob(nvs_handle_t, const char *, void *data, size_t *length) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return 1;
  f.read(static_cast<char *>(data), *length);
  return f.gcount() == static_cast<long>(*length) ? 0 : 1;
}
esp_err_t nvs_set_blob(nvs_handle_t, const char *, const void *data, size_t n) {
  auto p = static_cast<const uint8_t *>(data); pending.assign(p, p + n); return 0;
}
esp_err_t nvs_commit(nvs_handle_t) {
  if (fail_commit) return 1;
  std::ofstream f(path, std::ios::binary); f.write(reinterpret_cast<const char *>(pending.data()), pending.size());
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
    puts("PASS: replay rejected after process restart"); return 0;
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
  puts("PASS: real AES-CCM vector, tampering, nonce identity, truncation, plaintext, persistence failure and replay");
}
