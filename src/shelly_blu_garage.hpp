#pragma once

#ifdef SHELLY_GARAGE_BLE
#include <string>
namespace shelly {
namespace blu {
void Init();
void Invalidate();
bool Enabled();
bool Ready();
void GetInputs(int *closed, int *open);
std::string GetInfoJSON();
bool ValidMAC(const char *mac);
}  // namespace blu
}  // namespace shelly
#endif
