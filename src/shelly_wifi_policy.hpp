#pragma once

// The current ESP32-C3 + BLE build cannot safely use WIFI_PS_NONE.
// Other hardware profiles keep their existing 0/1/2 choices.
#ifndef SHELLY_WIFI_MIN_PS_MODE
#define SHELLY_WIFI_MIN_PS_MODE 0
#endif

namespace shelly {

constexpr int kWifiMinPowerSaveMode = SHELLY_WIFI_MIN_PS_MODE;
static_assert(kWifiMinPowerSaveMode >= 0 && kWifiMinPowerSaveMode <= 2,
              "Invalid Wi-Fi power save policy");

inline bool WifiPowerSaveModeAllowed(int mode) {
  return mode >= kWifiMinPowerSaveMode && mode <= 2;
}

inline int WifiBootPowerSaveMode(int stored_mode) {
  return WifiPowerSaveModeAllowed(stored_mode) ? stored_mode
                                             : kWifiMinPowerSaveMode;
}

}  // namespace shelly
