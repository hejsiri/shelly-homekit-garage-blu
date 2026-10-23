#pragma once
#ifdef SHELLY_GARAGE_BLE
#include <stddef.h>
#include <stdint.h>
namespace shelly {
namespace blu {
struct SecureRecord {
  uint32_t version;
  uint8_t mac[6];
  uint8_t key[16];
  uint8_t counter_seen;
  uint8_t reserved;
  uint32_t counter;
};
bool LoadRecord(int role, SecureRecord *record);
bool SaveRecord(int role, const SecureRecord &record);
bool AcceptSecure(int role, const uint8_t mac[6], const uint8_t *service,
                  size_t size, uint8_t *plain, size_t *plain_size);
bool SecureFor(int role, const uint8_t mac[6]);
void ResetReadings();
}  // namespace blu
}  // namespace shelly
#endif
