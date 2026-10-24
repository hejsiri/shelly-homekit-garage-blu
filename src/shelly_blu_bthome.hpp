#pragma once

#include <stddef.h>
#include <stdint.h>

namespace shelly {
namespace blu {

struct Reading {
  bool window_open = false;
  int battery = -1;
  bool motion = false;
  int button = -1;
  int packet_id = -1;
  int temperature10 = 0;
  int humidity = -1;
  int32_t illuminance100 = -1;  // BTHome 0x05: hundredths of a lux, -1 = absent.
  bool temperature_present = false;
};

// BTHome v2 service payload, after UUID 0xFCD2. Only the objects used by
// BLU Door/Window and Motion are accepted; unknown object lengths are rejected.
inline bool Decode(const uint8_t *data, size_t len, Reading *out, bool motion = false) {
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
      case 0x21:
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
    if (id == 0x05) {
      result.illuminance100 = int32_t(data[i]) | (int32_t(data[i + 1]) << 8) |
                             (int32_t(data[i + 2]) << 16);
    }
    if (id == (motion ? 0x2d : 0x21)) return false;
    if (id == (motion ? 0x21 : 0x2d)) {
      if (data[i] > 1) return false;
      if (motion) result.motion = data[i] != 0;
      else result.window_open = data[i] != 0;
      have_window = true;
    }
    i += size;
  }
  if (!have_window) return false;
  *out = result;
  return true;
}

// SBHT-003C: signed tenths of a degree and integer percent humidity.
inline bool DecodeHT(const uint8_t *data, size_t len, Reading *out) {
  if (len < 1 || (data[0] >> 5) != 2 || (data[0] & 0x1b)) return false;
  Reading result;
  bool temperature = false;
  int previous = -1;
  for (size_t i = 1; i < len;) {
    const uint8_t id = data[i++];
    if (id <= previous) return false;
    previous = id;
    size_t n = id == 0x45 ? 2 :
      (id == 0 || id == 1 || id == 0x15 || id == 0x1e || id == 0x2e || id == 0x3a || id == 0x64) ? 1 : 0;
    if (!n || n > len - i) return false;
    if (id == 1) { if (data[i] > 100) return false; result.battery = data[i]; }
    if (id == 0x2e) { if (data[i] > 100) return false; result.humidity = data[i]; }
    if (id == 0x45) {
      int value = int(data[i]) | (int(data[i + 1]) << 8);
      result.temperature10 = value >= 32768 ? value - 65536 : value;
      if (result.temperature10 < -1000 || result.temperature10 > 1000) return false;
      temperature = true;
      result.temperature_present = true;
    }
    i += n;
  }
  if (!temperature || result.humidity < 0) return false;
  *out = result;
  return true;
}

// Single-key BTHome Button1. No event means presence only.
inline bool DecodeButton(const uint8_t *data, size_t len, Reading *out) {
  if (!len || (data[0] >> 5) != 2 || (data[0] & 0x1b)) return false;
  Reading r;
  int previous = -1;
  bool have_data = false;
  for (size_t i = 1; i < len;) {
    const uint8_t id = data[i++];
    if (id <= previous) return false;
    previous = id;
    const size_t n = id == 0xf0 ? 2 : id == 0xf1 ? 4 : id == 0xf2 ? 3 :
      (id == 0 || id == 1 || id == 0x3a) ? 1 : 0;
    if (!n || n > len - i) return false;
    if (id == 0) r.packet_id = data[i];
    if (id == 1) { if (data[i] > 100) return false; r.battery = data[i]; have_data = true; }
    if (id == 0x3a) { r.button = data[i]; have_data = true; }
    i += n;
  }
  if (!have_data) return false;
  *out = r;
  return true;
}

struct ButtonEvents {
  int last_packet = -1;
  double last_press = -10;
  bool Accept(const Reading &r, double now) {
    if (r.packet_id < 0) return false;
    if (r.packet_id == last_packet) return false;
    last_packet = r.packet_id;
    if (r.button != 1 || now - last_press < 1.5) return false;
    last_press = now;
    return true;
  }
};

// HomeKit supports single, double and long presses. Keep this deduplication
// separate from the garage's single-press debounce.
struct ProgrammableButtonEvents {
  int last_packet = -1;
  int Accept(const Reading &r) {
    if (r.packet_id < 0 || r.packet_id == last_packet) return -1;
    last_packet = r.packet_id;
    switch (r.button) {
      case 1: return 0;
      case 2: return 1;
      case 4: return 2;
      default: return -1;
    }
  }
};

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
  int temperature10 = 0;
  int humidity = -1;
  int32_t illuminance100 = -1;
  double illuminance_last_seen = 0;
  bool temperature_seen = false, humidity_seen = false;
  double temperature_last_seen = 0, humidity_last_seen = 0;
  void Update(const Reading &reading, double now, int signal, int kind = 0) {
    seen = true;
    active = kind == 1 ? reading.motion : kind == 0 ? !reading.window_open : false;
    if (kind == 2) {
      if (reading.temperature_present) {
        temperature10 = reading.temperature10;
        temperature_seen = true;
        temperature_last_seen = now;
      }
      if (reading.humidity >= 0) {
        humidity = reading.humidity;
        humidity_seen = true;
        humidity_last_seen = now;
      }
    }
    if (reading.illuminance100 >= 0) {
      illuminance100 = reading.illuminance100;
      illuminance_last_seen = now;
    }
    last_seen = now;
    if (reading.battery >= 0) battery = reading.battery;
    rssi = signal;
  }
  bool Fresh(double now, int timeout) const {
    return seen && now >= last_seen && now - last_seen <= timeout;
  }
  bool IlluminanceFresh(double now, int timeout) const {
    return seen && illuminance100 >= 0 && now >= illuminance_last_seen &&
           now - illuminance_last_seen <= timeout;
  }
  bool HTFresh(bool temperature, double now, int timeout) const {
    const double at = temperature ? temperature_last_seen : humidity_last_seen;
    return (temperature ? temperature_seen : humidity_seen) && now >= at &&
           now - at <= timeout;
  }
};

inline bool Ready(const Sensor &closed, const Sensor &open, double now,
                  int timeout) {
  return closed.Fresh(now, timeout) && open.Fresh(now, timeout) &&
         !(closed.active && open.active);
}

}  // namespace blu
}  // namespace shelly
