#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "shelly_blu_bthome.hpp"

namespace shelly {
namespace blu {
// Xiaomi MiBeacon service data, without UUID. Explicit stock-device allowlist.
struct XiaomiFrame {
  uint16_t model;
  bool encrypted;
  uint8_t packet_id;
  const uint8_t *payload;
  size_t length;
  uint8_t nonce[12];
  const uint8_t *mic;
  uint32_t counter;
};
inline bool ParseXiaomiFrame(const uint8_t *data, size_t size,
                             const uint8_t mac[6], XiaomiFrame *out) {
  if (size < 5 || size > 31) return false;
  const uint16_t flags = data[0] | (uint16_t(data[1]) << 8);
  const unsigned version = flags >> 12;
  const uint16_t model = data[2] | (uint16_t(data[3]) << 8);
  const bool encrypted = flags & 8;
  if (version < 2 || version > 5 || !(flags & 0x40) || (flags & 0x80) ||
      (model != 0x01aa && model != 0x055b && model != 0x098b) ||
      (model == 0x01aa && encrypted) ||
      (model != 0x01aa && (!encrypted || version < 4))) return false;
  size_t offset = 5;
  if (flags & 0x10) {
    if (size - offset < 6) return false;
    for (size_t i = 0; i < 6; i++)
      if (data[offset + i] != mac[5 - i]) return false;
    offset += 6;
  }
  if (flags & 0x20) {
    if (offset == size) return false;
    const uint8_t capability = data[offset++];
    if (capability & 0x20) {
      if (offset == size) return false;
      offset++;
    }
  }
  XiaomiFrame frame = {};
  frame.model = model;
  frame.encrypted = encrypted;
  frame.packet_id = data[4];
  frame.payload = data + offset;
  frame.length = size - offset;
  if (encrypted) {
    if (frame.length < 11) return false; // Object (at least 4), counter (3), MIC (4).
    frame.length -= 7;
    const uint8_t *extended = data + size - 7;
    frame.mic = data + size - 4;
    for (size_t i = 0; i < 6; i++) frame.nonce[i] = mac[5 - i];
    memcpy(frame.nonce + 6, data + 2, 3);
    memcpy(frame.nonce + 9, extended, 3);
    frame.counter = uint32_t(data[4]) | (uint32_t(extended[0]) << 8) |
                    (uint32_t(extended[1]) << 16) | (uint32_t(extended[2]) << 24);
  }
  *out = frame;
  return true;
}
// Same TLV measurement objects in plaintext LYWSDCGQ and decrypted LYWSD03MMC.
inline bool DecodeXiaomiObjects(const uint8_t *data, size_t size, Reading *out,
                                uint16_t model = 0x055b) {
  size_t offset = 0;
  Reading reading;
  bool any = false;
  unsigned fields = 0;
  while (offset < size) {
    if (size - offset < 3) return false;
    const uint16_t type = data[offset] | (uint16_t(data[offset + 1]) << 8);
    const size_t length = data[offset + 2];
    offset += 3;
    if (!length || length > size - offset) return false;
    const uint8_t *value = data + offset;
    unsigned mask = 0;
    if (model == 0x098b && type == 0x1019) {
      if (length != 1 || value[0] > 3) return false;
      reading.window_reset = value[0] == 3;
      reading.window_present = !reading.window_reset;
      reading.window_open = value[0] != 1; // 0=open, 1=closed, 2=left open.
      mask = 8;
    } else if (model == 0x098b && (type == 0x1018 || type == 0x1007)) {
      if (length != (type == 0x1018 ? 1u : 3u)) return false;
      if (type == 0x1018 && value[0] > 1) return false;
      const unsigned raw = type == 0x1018 ? value[0] :
          unsigned(value[0]) | (unsigned(value[1]) << 8) | (unsigned(value[2]) << 16);
      reading.light = type == 0x1018 ? raw : raw == 100;
      mask = 16;
    } else if (model != 0x098b && (type == 0x1004 || type == 0x1006 || type == 0x100d)) {
      if (length != (type == 0x100d ? 4u : 2u)) return false;
      if (type != 0x1006) {
        const int raw = value[0] | (int(value[1]) << 8);
        reading.temperature10 = raw >= 32768 ? raw - 65536 : raw;
        if (reading.temperature10 < -1000 || reading.temperature10 > 1000)
          return false;
        reading.temperature_present = true;
        mask |= 1;
      }
      if (type != 0x1004) {
        const size_t at = type == 0x100d ? 2 : 0;
        const unsigned humidity10 = value[at] | (unsigned(value[at + 1]) << 8);
        if (humidity10 > 1000) return false;
        reading.humidity = (humidity10 + 5) / 10;
        mask |= 2;
      }
    } else if (type == 0x100a) {
      if (length != 1 || value[0] > 100) return false;
      reading.battery = value[0];
      mask = 4;
    }
    // Unknown objects have an explicit length; never treat them as readings.
    if (fields & mask) return false;
    fields |= mask;
    any = any || mask;
    offset += length;
  }
  if (!any) return false;
  *out = reading;
  return true;
}

inline bool DecodeXiaomi(const uint8_t *data, size_t size,
                         const uint8_t mac[6], Reading *out) {
  XiaomiFrame frame;
  if (!ParseXiaomiFrame(data, size, mac, &frame) || frame.encrypted ||
      !DecodeXiaomiObjects(frame.payload, frame.length, out)) return false;
  out->packet_id = frame.packet_id;
  return true;
}
inline bool XiaomiServiceData(const uint8_t *data, size_t size,
                              const uint8_t **service, size_t *length) {
  bool found = false;
  for (size_t offset = 0; offset < size;) {
    const size_t n = data[offset++];
    if (!n) break;
    if (n > size - offset) return false;
    if (n >= 3 && data[offset] == 0x16 &&
        data[offset + 1] == 0x95 && data[offset + 2] == 0xfe) {
      if (found) return false;
      *service = data + offset + 3;
      *length = n - 3;
      found = true;
    }
    offset += n;
  }
  return found;
}
inline bool DecodeXiaomiAdvertisement(const uint8_t *data, size_t size,
                                      const uint8_t mac[6], Reading *out) {
  const uint8_t *service;
  size_t length;
  return XiaomiServiceData(data, size, &service, &length) &&
         DecodeXiaomi(service, length, mac, out);
}
inline uint16_t XiaomiAdvertisementModel(const uint8_t *data, size_t size,
                                         const uint8_t mac[6]) {
  const uint8_t *service;
  size_t length;
  XiaomiFrame frame;
  if (!XiaomiServiceData(data, size, &service, &length) ||
      !ParseXiaomiFrame(service, length, mac, &frame)) return 0;
  Reading reading;
  return frame.encrypted || DecodeXiaomi(service, length, mac, &reading) ? frame.model : 0;
}
inline bool ParseXiaomiBindkey(const char *text, uint8_t key[16]) {
  if (!text || strlen(text) != 32) return false;
  for (size_t i = 0; i < 16; i++) {
    unsigned value = 0;
    for (size_t j = 0; j < 2; j++) {
      const char c = text[i * 2 + j];
      const int digit = c >= '0' && c <= '9' ? c - '0' :
                        c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                        c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
      if (digit < 0) return false;
      value = value * 16 + digit;
    }
    key[i] = value;
  }
  return true;
}
}  // namespace blu
}  // namespace shelly
