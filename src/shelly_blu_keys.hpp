#pragma once
#ifdef SHELLY_GARAGE_BLE
#include <stddef.h>
#include <stdint.h>
namespace shelly {
namespace blu {
struct Reading;
enum : uint8_t { kProtocolBTHome = 0, kProtocolXiaomi = 1, kProtocolXiaomiGATT = 4, kProtocolMiBindkey = 5, kProtocolQingping = 6, kProtocolQingpingBindkey = 7, kProtocolClockBindkey = 8, kProtocolClockGATT = 9 };
struct SecureRecord {
  uint32_t version;
  uint8_t mac[6];
  uint8_t key[16];
  uint8_t counter_seen;
  uint8_t protocol;  // Former reserved byte: existing encrypted records are 0.
  uint32_t counter;
};
bool LoadRecord(int role, SecureRecord *record);
bool DeleteRecord(int role);
bool QingpingFor(int role, const uint8_t mac[6]);
bool AcceptQingping(int role,const uint8_t mac[6],const uint8_t *data,size_t size,
                    Reading *reading,bool *state);
bool XiaomiClockFor(int role, const uint8_t mac[6]);
bool XiaomiBindkeyFor(int role, const uint8_t mac[6]);
bool DecryptMi(const SecureRecord &record, const uint8_t *data, size_t size,
               uint8_t *plain, size_t *length, uint32_t *counter, uint16_t *model);
bool AcceptMi(int role, const uint8_t mac[6], const uint8_t *data, size_t size,
              uint8_t *plain, size_t *length, uint16_t *model);
bool SaveRecord(int role, const SecureRecord &record);
bool AcceptSecure(int role, const uint8_t mac[6], const uint8_t *service,
                  size_t size, uint8_t *plain, size_t *plain_size);
bool SecureFor(int role, const uint8_t mac[6]);
bool XiaomiFor(int role, const uint8_t mac[6]);
bool XiaomiGATTFor(int role, const uint8_t mac[6]);
// MCCGQ02HL battery telemetry, scoped by endpoint role and MAC.
void CacheBattery(int role, const uint8_t mac[6], int percent, int64_t epoch, double uptime);
int CachedBattery(int role, const uint8_t mac[6], int64_t epoch, double uptime);
void ForgetBattery(int role);
void CacheContact(int role, const uint8_t mac[6], bool active, int rssi, int64_t epoch, double uptime);
bool CachedContact(int role, const uint8_t mac[6], int64_t epoch, double uptime, bool *active, int *rssi, double *age);
void ForgetContact(int role);
void ResetReadings();
void ForgetReading(int role);
}  // namespace blu
}  // namespace shelly
#endif
