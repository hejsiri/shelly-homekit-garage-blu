#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace shelly {
namespace blu {
// Service data includes UUID, info, ciphertext, uint32 counter and 4-byte MIC.
struct Envelope {
  const uint8_t *ciphertext;
  size_t length;
  const uint8_t *mic;
  uint32_t counter;
  uint8_t nonce[13];
  uint8_t info;
};
inline bool ParseEnvelope(const uint8_t *data, size_t size,
                          const uint8_t mac[6], Envelope *out) {
  if (size < 12 || size > 31 || data[0] != 0xd2 || data[1] != 0xfc ||
      (data[2] >> 5) != 2 || !(data[2] & 1) || (data[2] & 0x1a))
    return false;
  Envelope e = {};
  e.info = data[2];
  e.ciphertext = data + 3;
  e.length = size - 11;
  e.mic = data + size - 4;
  const uint8_t *counter = data + size - 8;
  e.counter = uint32_t(counter[0]) | (uint32_t(counter[1]) << 8) |
              (uint32_t(counter[2]) << 16) | (uint32_t(counter[3]) << 24);
  memcpy(e.nonce, mac, 6);
  memcpy(e.nonce + 6, data, 3);
  memcpy(e.nonce + 9, counter, 4);
  *out = e;
  return true;
}
inline bool NewCounter(bool seen, uint32_t previous, uint32_t incoming) {
  // No wrap acceptance: counter exhaustion requires a new key.
  return !seen || incoming > previous;
}
}  // namespace blu
}  // namespace shelly
