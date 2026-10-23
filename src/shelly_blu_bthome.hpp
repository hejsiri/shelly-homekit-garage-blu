#pragma once

#include <stddef.h>
#include <stdint.h>

namespace shelly {
namespace blu {

struct Reading {
  bool window_open = false;
  int battery = -1;
};

// BTHome v2 service payload, after UUID 0xFCD2. Only the objects used by
// BLU Door/Window are accepted; never guess the length of an unknown object.
inline bool Decode(const uint8_t *data, size_t len, Reading *out) {
  if (len < 1 || (data[0] >> 5) != 2 || (data[0] & 0x1b)) return false;
  Reading result;
  bool have_window = false;
  int previous = -1;
  for (size_t i = 1; i < len;) {
    const uint8_t id = data[i++];
    if (id <= previous) return false;
    previous = id;
    size_t size;
    switch (id) {
      case 0x00:
      case 0x01:
      case 0x2d:
      case 0x3a:
        size = 1;
        break;
      case 0x05:
        size = 3;
        break;
      case 0x3f:
        size = 2;
        break;
      default:
        return false;
    }
    if (size > len - i) return false;
    if (id == 0x01) {
      if (data[i] > 100) return false;
      result.battery = data[i];
    }
    if (id == 0x2d) {
      if (data[i] > 1) return false;
      result.window_open = data[i] != 0;
      have_window = true;
    }
    i += size;
  }
  if (!have_window) return false;
  *out = result;
  return true;
}

inline bool DecodeAdvertisement(const uint8_t *data, size_t len, Reading *out) {
  bool found = false;
  Reading reading;
  for (size_t i = 0; i < len;) {
    size_t n = data[i++];
    if (n == 0) break;
    if (n > len - i) return false;
    if (n >= 3 && data[i] == 0x16 && data[i + 1] == 0xd2 &&
        data[i + 2] == 0xfc) {
      if (found || !Decode(data + i + 3, n - 3, &reading)) return false;
      found = true;
    }
    i += n;
  }
  if (found) *out = reading;
  return found;
}

struct Sensor {
  bool seen = false;
  bool active = false;  // Magnet present = endpoint reached.
  double last_seen = 0;
  int battery = -1;
  int rssi = 0;
  void Update(const Reading &reading, double now, int signal) {
    seen = true;
    active = !reading.window_open;
    last_seen = now;
    if (reading.battery >= 0) battery = reading.battery;
    rssi = signal;
  }
  bool Fresh(double now, int timeout) const {
    return seen && now >= last_seen && now - last_seen <= timeout;
  }
};

inline bool Ready(const Sensor &closed, const Sensor &open, double now,
                  int timeout) {
  return closed.Fresh(now, timeout) && open.Fresh(now, timeout) &&
         !(closed.active && open.active);
}

}  // namespace blu
}  // namespace shelly
