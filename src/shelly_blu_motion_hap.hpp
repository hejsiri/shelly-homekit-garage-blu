#pragma once
#ifdef SHELLY_GARAGE_BLE
#include <memory>
#include "mgos_hap_accessory.hpp"
#include "mgos_hap_service.hpp"
namespace shelly { namespace blu {
std::unique_ptr<mgos::hap::Service> CreateMotionService();
std::unique_ptr<mgos::hap::Service> CreateHTService(int role, bool temperature);
void AddButtonAccessories(std::vector<std::unique_ptr<mgos::hap::Accessory>> *accessories,
                          HAPAccessoryServerRef *server);
} }
#endif
