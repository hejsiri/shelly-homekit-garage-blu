#pragma once
#ifdef SHELLY_GARAGE_BLE
namespace shelly {
namespace blu {
void PairingInit();
bool PairingScan();
bool PairingConnecting();
bool PairingBusy();
void PairingAdvertisement(void *result);
void PairingVerified(int role);
}  // namespace blu
}  // namespace shelly
#endif
