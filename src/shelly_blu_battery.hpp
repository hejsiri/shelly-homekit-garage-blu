#pragma once
#include <stdint.h>
#include <string.h>
namespace shelly { namespace blu {
// Persist only battery telemetry. Contact state is never restored from this cache.
struct BatteryRecord {
  uint32_t version = 1;
  uint8_t mac[6] = {};
  uint8_t percent = 255;
  int64_t received_at = 0;
};
struct BatteryCache {
  static constexpr int64_t kLifetime = 24 * 60 * 60;
  BatteryRecord record;
  bool loaded = false, anchored = false, dirty = false;
  double received_uptime = 0;
  static bool ClockReady(int64_t epoch) { return epoch >= 1577836800; }
  bool Matches(const uint8_t mac[6]) const {
    return record.version == 1 && record.percent <= 100 &&
           memcmp(record.mac, mac, 6) == 0;
  }
  void Receive(const uint8_t mac[6], int percent, int64_t epoch, double uptime) {
    record = BatteryRecord();
    memcpy(record.mac, mac, 6); record.percent = percent;
    record.received_at = ClockReady(epoch) ? epoch : 0;
    loaded = anchored = dirty = true; received_uptime = uptime;
  }
  int Read(const uint8_t mac[6], int64_t epoch, double uptime) {
    if (!Matches(mac)) return -1;
    if (!anchored) {
      // After boot, wait for wall-clock synchronization to account for downtime.
      if (!ClockReady(epoch) || !ClockReady(record.received_at) ||
          epoch < record.received_at) return -1;
      const int64_t age = epoch - record.received_at;
      if (age >= kLifetime) { record.percent = 255; return -1; }
      received_uptime = uptime - age; anchored = true;
    }
    const double age = uptime - received_uptime;
    if (age < 0 || age >= kLifetime) { record.percent = 255; dirty = false; return -1; }
    // A sample received before NTP can be saved once a reliable clock arrives.
    if (!ClockReady(record.received_at) && ClockReady(epoch)) {
      record.received_at = epoch - static_cast<int64_t>(age); dirty = true;
    }
    return record.percent;
  }
};
} }
