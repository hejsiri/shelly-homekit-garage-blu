#pragma once

#ifdef SHELLY_GARAGE_BLE
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
bool ConsumeButtonPress();
const Sensor &ReadingFor(int role);
bool SensorReady(int role);
bool SensorPaired(int role);
}  // namespace blu
}  // namespace shelly
#endif
