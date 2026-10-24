#pragma once

#ifdef SHELLY_GARAGE_BLE
#include <functional>
#include <string>
#include "shelly_blu_bthome.hpp"
namespace shelly {
namespace blu {
void Init();
void Invalidate();
bool Enabled();
bool Ready();
void GetInputs(int *closed, int *open);
std::string GetInfoJSON();
bool ValidMAC(const char *mac);
const char *SensorMAC(int role);
const char *SensorName(int role);
void SetSensorName(int role, const char *name);
const Sensor &MotionReading();
bool MotionReady();
bool MotionLightReady();
bool ConsumeButtonPress();
void SetButtonEventHandler(std::function<void(uint8_t)> handler);
const char *CarName();
bool CarPresenceReady();
bool CarPresent();
const Sensor &ReadingFor(int role);
bool SensorReady(int role);
bool HTReady(int role, bool temperature);
int SensorTimeout(int role);
void SetDeviceReading(int role, const Sensor &reading);
bool SensorPaired(int role);
}  // namespace blu
}  // namespace shelly
#endif
