#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "shelly_blu_bthome.hpp"

namespace shelly {
namespace blu {
// Stock LYWSDCGQ/01ZM (MJ_HT_V1), MiBeacon product 0x01aa, service 0xfe95.
// This is a passive, unencrypted thermometer, never a garage control input.
// Wire format reference: Bluetooth-Devices/xiaomi-ble, test_Xiaomi_LYWSDCGQ.
inline bool DecodeXiaomi(const uint8_t *data, size_t size,
                         const uint8_t mac[6], Reading *out) {
  if (size < 5) return false;
  const uint16_t flags = data[0] | (uint16_t(data[1]) << 8);
  const unsigned version = flags >> 12;
  if (version < 2 || version > 5 || !(flags & 0x40) ||
      (flags & (0x08 | 0x80)) || data[2] != 0xaa || data[3] != 0x01)
    return false;
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
      offset++;  // Optional I/O capability.
    }
  }
  Reading reading;
  reading.packet_id = data[4];
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
    if (type == 0x1004 || type == 0x1006 || type == 0x100d) {
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

inline bool DecodeXiaomiAdvertisement(const uint8_t *data, size_t size,
                                      const uint8_t mac[6], Reading *out) {
  bool found = false;
  Reading reading;
  for (size_t offset = 0; offset < size;) {
    const size_t length = data[offset++];
    if (!length) break;
    if (length > size - offset) return false;
    if (length >= 3 && data[offset] == 0x16 &&
        data[offset + 1] == 0x95 && data[offset + 2] == 0xfe) {
      if (found || !DecodeXiaomi(data + offset + 3, length - 3, mac, &reading))
        return false;
      found = true;
    }
    offset += length;
  }
  if (found) *out = reading;
  return found;
}
}  // namespace blu
}  // namespace shelly
