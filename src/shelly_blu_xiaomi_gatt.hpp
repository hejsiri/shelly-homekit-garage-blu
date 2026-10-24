#pragma once
#include "shelly_blu_bthome.hpp"

namespace shelly {
namespace blu {
// Stock LYWSD03MMC identification only; encrypted MiBeacon data is not decoded.
inline bool Xiaomi03Advertisement(const uint8_t *data, size_t size) {
  for (size_t i = 0; i < size;) {
    const size_t n = data[i++];
    if (!n) break;
    if (n > size - i) return false;
    if (n >= 7 && data[i] == 0x16 && data[i + 1] == 0x95 &&
        data[i + 2] == 0xfe && data[i + 5] == 0x5b && data[i + 6] == 0x05)
      return true;
    static const char name[] = "LYWSD03MMC";
    if ((data[i] == 8 || data[i] == 9) && n == sizeof(name)) {
      bool match = true;
      for (size_t j = 0; j < sizeof(name) - 1; j++)
        match = match && data[i + 1 + j] == uint8_t(name[j]);
      if (match) return true;
    }
    i += n;
  }
  return false;
}
inline bool DecodeXiaomi03GATT(const uint8_t *data, size_t size, Reading *out,
                              int *voltage_mv) {
  if (size != 5) return false;
  const int temperature = int16_t(uint16_t(data[0]) | uint16_t(data[1]) << 8);
  const int voltage = uint16_t(data[3]) | uint16_t(data[4]) << 8;
  if (temperature < -5000 || temperature > 10000 || data[2] > 100 ||
      voltage < 1800 || voltage > 3600) return false;
  Reading reading;
  reading.temperature_present = true;
  reading.temperature10 = (temperature + (temperature >= 0 ? 5 : -5)) / 10;
  reading.humidity = data[2];
  // An estimate from cell voltage, not the stock battery-percent characteristic.
  reading.battery = (voltage - 2100 + 5) / 10;
  if (reading.battery < 0) reading.battery = 0;
  if (reading.battery > 100) reading.battery = 100;
  *out = reading;
  *voltage_mv = voltage;
  return true;
}
}  // namespace blu
}  // namespace shelly

#ifdef SHELLY_GARAGE_BLE
#include <functional>
#include <string>
struct mgos_bt_addr;
namespace shelly {
namespace blu {
using XiaomiGATTCallback = std::function<void(const Sensor &, int, const std::string &)>;
void XiaomiGATTInit();
bool XiaomiGATTStart(int role, const mgos_bt_addr &peer, XiaomiGATTCallback callback);
void XiaomiGATTCancel();
bool XiaomiGATTConnecting();
bool XiaomiGATTScanPaused();
void XiaomiGATTSetGarageMoving(bool moving);
std::string XiaomiGATTStatusJSON(int role);
}  // namespace blu
}  // namespace shelly
#endif
