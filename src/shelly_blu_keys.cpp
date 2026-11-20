#include "shelly_blu_keys.hpp"
#ifdef SHELLY_GARAGE_BLE
#include <string.h>
#include "mbedtls/ccm.h"
#include "nvs.h"
#include "shelly_blu_secure.hpp"
#include "shelly_blu_mibeacon.hpp"
#include "shelly_blu_battery.hpp"
#include "shelly_blu_contact.hpp"

namespace shelly {
namespace blu {
namespace {
SecureRecord cache[7] = {};
bool loaded[7] = {};
const char *Name(int role) {
  static const char *names[] = {"closed", "open", "motion", "indoor", "outdoor", "button", "smoke"};
  return names[role];
}
bool ValidProtocol(int role, uint8_t protocol) {
  return (role>=0 && role<=4 && (protocol==kProtocolQingping || protocol==kProtocolQingpingBindkey)) ||
         (((role >= 0 && role <= 4) || role == 6) && protocol == kProtocolMiBindkey) ||
         (role < 6 && protocol == kProtocolBTHome) ||
         ((role == 3 || role == 4) &&
          (protocol == kProtocolXiaomi || protocol == kProtocolXiaomiGATT || protocol == kProtocolClockBindkey || protocol == kProtocolClockGATT));
}
}  // namespace
namespace {
BatteryCache batteries[2];
const char *BatteryName(int role) { return role == 0 ? "bat_closed" : "bat_open"; }
void LoadBattery(int role) {
  auto &entry = batteries[role];
  if (entry.loaded) return;
  entry.loaded = true;
  nvs_handle_t h;
  if (nvs_open("blu_battery", NVS_READONLY, &h) != ESP_OK) return;
  BatteryRecord record; size_t size = sizeof(record);
  const auto err = nvs_get_blob(h, BatteryName(role), &record, &size);
  nvs_close(h);
  if (err == ESP_OK && size == sizeof(record) && record.version == 1 && record.percent <= 100)
    entry.record = record;
}
void SaveBattery(int role) {
  auto &entry = batteries[role];
  if (!entry.dirty || !BatteryCache::ClockReady(entry.record.received_at)) return;
  // One attempt per received sample (or initial clock synchronization).
  entry.dirty = false;
  nvs_handle_t h;
  if (nvs_open("blu_battery", NVS_READWRITE, &h) != ESP_OK) return;
  if (nvs_set_blob(h, BatteryName(role), &entry.record, sizeof(entry.record)) == ESP_OK)
    nvs_commit(h);
  nvs_close(h);
}
}
void CacheBattery(int role, const uint8_t mac[6], int percent, int64_t epoch, double uptime) {
  if (role < 0 || role > 1 || percent < 0 || percent > 100 || !XiaomiBindkeyFor(role, mac) || QingpingFor(role, mac)) return;
  batteries[role].Receive(mac, percent, epoch, uptime); SaveBattery(role);
}
int CachedBattery(int role, const uint8_t mac[6], int64_t epoch, double uptime) {
  if (role < 0 || role > 1 || !XiaomiBindkeyFor(role, mac) || QingpingFor(role, mac)) return -1;
  LoadBattery(role);
  int value = batteries[role].Read(mac, epoch, uptime); SaveBattery(role); return value;
}
void ForgetBattery(int role) {
  if (role < 0 || role > 1) return;
  batteries[role] = BatteryCache(); batteries[role].loaded = true;
  nvs_handle_t h;
  if (nvs_open("blu_battery", NVS_READWRITE, &h) != ESP_OK) return;
  const auto err = nvs_erase_key(h, BatteryName(role));
  if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) nvs_commit(h);
  nvs_close(h);
}
namespace {
ContactCache contacts[2];
const char *ContactName(int role) { return role == 0 ? "state_closed" : "state_open"; }
bool ContactScope(int role, const uint8_t mac[6]) {
  return role >= 0 && role < 2 && XiaomiBindkeyFor(role, mac) && !QingpingFor(role, mac);
}
void SaveContact(int role) {
  auto &entry = contacts[role];
  if (!entry.dirty || !BatteryCache::ClockReady(entry.record.received_at)) return;
  entry.dirty = false;
  nvs_handle_t h;
  if (nvs_open("blu_contact", NVS_READWRITE, &h) != ESP_OK) return;
  if (nvs_set_blob(h, ContactName(role), &entry.record, sizeof(entry.record)) == ESP_OK) nvs_commit(h);
  nvs_close(h);
}
}
void CacheContact(int role, const uint8_t mac[6], bool active, int rssi, int64_t epoch, double uptime) {
  if (!ContactScope(role, mac)) return;
  contacts[role].Receive(mac, active, rssi, epoch, uptime); SaveContact(role);
}
bool CachedContact(int role, const uint8_t mac[6], int64_t epoch, double uptime, bool *active, int *rssi, double *age) {
  if (!ContactScope(role, mac)) return false;
  auto &entry = contacts[role];
  if (!entry.loaded) {
    entry.loaded = true;
    nvs_handle_t h;
    if (nvs_open("blu_contact", NVS_READONLY, &h) == ESP_OK) {
      ContactRecord record; size_t size = sizeof(record);
      auto err = nvs_get_blob(h, ContactName(role), &record, &size); nvs_close(h);
      if (err == ESP_OK && size == sizeof(record) && record.version == 1 && record.active <= 1) entry.record = record;
    }
  }
  const bool result = entry.Read(mac, epoch, uptime, age); SaveContact(role);
  if (result) { *active = entry.record.active; *rssi = entry.record.rssi; }
  return result;
}
void ForgetContact(int role) {
  if (role < 0 || role > 1) return;
  contacts[role] = ContactCache(); contacts[role].loaded = true;
  nvs_handle_t h;
  if (nvs_open("blu_contact", NVS_READWRITE, &h) != ESP_OK) return;
  const auto err = nvs_erase_key(h, ContactName(role));
  if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) nvs_commit(h);
  nvs_close(h);
}
bool LoadRecord(int role, SecureRecord *record) {
  if (role < 0 || role > 6) return false;
  if (!loaded[role]) {
    nvs_handle_t handle;
    if (nvs_open("blu_secure", NVS_READONLY, &handle) != ESP_OK) return false;
    size_t length = sizeof(SecureRecord);
    esp_err_t err = nvs_get_blob(handle, Name(role), &cache[role], &length);
    nvs_close(handle);
    if (err != ESP_OK || length != sizeof(SecureRecord) ||
        cache[role].version != 1 || !ValidProtocol(role, cache[role].protocol))
      return false;
    loaded[role] = true;
  }
  *record = cache[role];
  return true;
}
bool SaveRecord(int role, const SecureRecord &record) {
  if (role < 0 || role > 6 || record.version != 1 ||
      !ValidProtocol(role, record.protocol)) return false;
  nvs_handle_t handle;
  if (nvs_open("blu_secure", NVS_READWRITE, &handle) != ESP_OK) return false;
  esp_err_t err = nvs_set_blob(handle, Name(role), &record, sizeof(record));
  if (err == ESP_OK) err = nvs_commit(handle);
  nvs_close(handle);
  if (err != ESP_OK) return false;
  cache[role] = record;
  loaded[role] = true;
  return true;
}
bool DeleteRecord(int role) {
  if (role < 0 || role > 6) return false;
  nvs_handle_t handle;
  if (nvs_open("blu_secure", NVS_READWRITE, &handle) != ESP_OK) return false;
  esp_err_t err = nvs_erase_key(handle, Name(role));
  if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
  if (err == ESP_OK) err = nvs_commit(handle);
  nvs_close(handle);
  if (err != ESP_OK) return false;
  memset(&cache[role], 0, sizeof(cache[role]));
  loaded[role] = false;
  return true;
}
bool SecureFor(int role, const uint8_t mac[6]) {
  SecureRecord record;
  return LoadRecord(role, &record) && record.protocol == kProtocolBTHome &&
         memcmp(record.mac, mac, 6) == 0;
}
bool XiaomiFor(int role, const uint8_t mac[6]) {
  SecureRecord record;
  return (role == 3 || role == 4) && LoadRecord(role, &record) &&
         record.protocol == kProtocolXiaomi && memcmp(record.mac, mac, 6) == 0;
}
bool XiaomiGATTFor(int role, const uint8_t mac[6]) {
  SecureRecord record;
  return (role == 3 || role == 4) && LoadRecord(role, &record) &&
         (record.protocol == kProtocolXiaomiGATT || record.protocol == kProtocolClockGATT) && memcmp(record.mac, mac, 6) == 0;
}
bool QingpingFor(int role,const uint8_t mac[6]) {
  SecureRecord r;
  return LoadRecord(role,&r) && (r.protocol==kProtocolQingping || r.protocol==kProtocolQingpingBindkey) && memcmp(r.mac,mac,6)==0;
}
bool AcceptQingping(int role,const uint8_t mac[6],const uint8_t *data,size_t size,
                    Reading *reading,bool *state) {
  SecureRecord r;uint16_t model;
  if(!LoadRecord(role,&r) || r.protocol!=kProtocolQingping || memcmp(r.mac,mac,6)!=0 ||
     !DecodeQingping(data,size,mac,&model,reading,state))return false;
  return MiRoleModel(role,model) && model==(uint16_t(r.key[0]) | uint16_t(r.key[1])<<8);
}
bool XiaomiClockFor(int role, const uint8_t mac[6]) {
  SecureRecord r;
  return LoadRecord(role, &r) && (r.protocol == kProtocolClockBindkey || r.protocol == kProtocolClockGATT) && memcmp(r.mac, mac, 6) == 0;
}
bool XiaomiBindkeyFor(int role, const uint8_t mac[6]) {
  SecureRecord r;
  return LoadRecord(role, &r) && (r.protocol == kProtocolMiBindkey || r.protocol == kProtocolClockBindkey || r.protocol == kProtocolQingpingBindkey) &&
         memcmp(r.mac, mac, 6) == 0;
}
bool DecryptMi(const SecureRecord &record, const uint8_t *data, size_t size,
               uint8_t *plain, size_t *length, uint32_t *counter, uint16_t *model) {
  MiEnvelope e;
  if ((record.protocol != kProtocolMiBindkey && record.protocol != kProtocolClockBindkey && record.protocol != kProtocolQingpingBindkey) ||
      !ParseMiEnvelope(data, size, record.mac, &e) || *length < e.length) return false;
  mbedtls_ccm_context ctx; mbedtls_ccm_init(&ctx);
  int result = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, record.key, 128);
  const uint8_t aad = 0x11;
  if (result == 0) result = mbedtls_ccm_auth_decrypt(&ctx, e.length,
      e.nonce, sizeof(e.nonce), &aad, 1, e.ciphertext, plain, e.mic, 4);
  mbedtls_ccm_free(&ctx);
  if (result != 0) return false;
  *length=e.length; *counter=e.counter; *model=e.model; return true;
}
bool AcceptMi(int role, const uint8_t mac[6], const uint8_t *data, size_t size,
              uint8_t *plain, size_t *length, uint16_t *model) {
  SecureRecord record; uint32_t counter;
  if (!LoadRecord(role,&record) || memcmp(record.mac,mac,6) != 0 ||
      !DecryptMi(record,data,size,plain,length,&counter,model) ||
      !NewCounter(record.counter_seen,record.counter,counter) ||
      (!MiRoleModel(role,*model) ||
       (XiaomiClockModel(*model) != (record.protocol == kProtocolClockBindkey)) ||
       (record.protocol==kProtocolQingpingBindkey ? !QingpingModel(*model) : QingpingModel(*model)))) return false;
  Reading reading; bool contact;
  if (!MiObjects(*model,plain,*length,&reading,&contact)) return false;
  record.counter_seen=true; record.counter=counter;
  return SaveRecord(role,record);
}
bool AcceptSecure(int role, const uint8_t mac[6], const uint8_t *service,
                  size_t size, uint8_t *plain, size_t *plain_size) {
  SecureRecord record;
  Envelope envelope;
  if (!LoadRecord(role, &record) || record.protocol != kProtocolBTHome ||
      memcmp(record.mac, mac, 6) != 0 ||
      !ParseEnvelope(service, size, mac, &envelope) ||
      !NewCounter(record.counter_seen, record.counter, envelope.counter) ||
      *plain_size < envelope.length + 1)
    return false;
  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);
  int result = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, record.key, 128);
  if (result == 0) {
    result = mbedtls_ccm_auth_decrypt(&ctx, envelope.length, envelope.nonce, 13,
                                      nullptr, 0, envelope.ciphertext,
                                      plain + 1, envelope.mic, 4);
  }
  mbedtls_ccm_free(&ctx);
  if (result != 0) return false;
  // Persist before exposing any decoded state, including across power loss.
  record.counter_seen = true;
  record.counter = envelope.counter;
  if (!SaveRecord(role, record)) return false;
  plain[0] = envelope.info & ~1;
  *plain_size = envelope.length + 1;
  return true;
}
}  // namespace blu
}  // namespace shelly
#endif
