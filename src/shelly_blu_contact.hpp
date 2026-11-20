#pragma once
#include <stdint.h>
#include <string.h>
#include "shelly_blu_battery.hpp"
namespace shelly { namespace blu {
struct ContactRecord {
  uint32_t version = 1;
  uint8_t mac[6] = {};
  uint8_t active = 255;
  int16_t rssi = 0;
  int64_t received_at = 0;
};
struct ContactCache {
  ContactRecord record;
  bool loaded = false, anchored = false, dirty = false;
  double received_uptime = 0;
  void Receive(const uint8_t mac[6], bool active, int rssi, int64_t epoch, double uptime) {
    record = ContactRecord(); memcpy(record.mac, mac, 6);
    record.active = active; record.rssi = rssi;
    record.received_at = BatteryCache::ClockReady(epoch) ? epoch : 0;
    loaded = anchored = dirty = true; received_uptime = uptime;
  }
  bool Read(const uint8_t mac[6], int64_t epoch, double uptime, double *age) {
    if (record.version != 1 || record.active > 1 || memcmp(record.mac, mac, 6)) return false;
    if (!anchored) {
      if (!BatteryCache::ClockReady(epoch) || !BatteryCache::ClockReady(record.received_at) || epoch < record.received_at) return false;
      received_uptime = uptime - (epoch - record.received_at); anchored = true;
    }
    *age = uptime - received_uptime;
    if (*age < 0) return false;
    if (!BatteryCache::ClockReady(record.received_at) && BatteryCache::ClockReady(epoch)) {
      record.received_at = epoch - static_cast<int64_t>(*age); dirty = true;
    }
    return true;
  }
};
} }
