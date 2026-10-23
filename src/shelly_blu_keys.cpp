#include "shelly_blu_keys.hpp"
#ifdef SHELLY_GARAGE_BLE
#include <string.h>
#include "mbedtls/ccm.h"
#include "nvs.h"
#include "shelly_blu_secure.hpp"

namespace shelly {
namespace blu {
namespace {
SecureRecord cache[2] = {};
bool loaded[2] = {};
const char *Name(int role) {
  return role == 0 ? "closed" : "open";
}
}  // namespace
bool LoadRecord(int role, SecureRecord *record) {
  if (role < 0 || role > 1) return false;
  if (!loaded[role]) {
    nvs_handle_t handle;
    if (nvs_open("blu_secure", NVS_READONLY, &handle) != ESP_OK) return false;
    size_t length = sizeof(SecureRecord);
    esp_err_t err = nvs_get_blob(handle, Name(role), &cache[role], &length);
    nvs_close(handle);
    if (err != ESP_OK || length != sizeof(SecureRecord) ||
        cache[role].version != 1)
      return false;
    loaded[role] = true;
  }
  *record = cache[role];
  return true;
}
bool SaveRecord(int role, const SecureRecord &record) {
  if (role < 0 || role > 1) return false;
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
bool SecureFor(int role, const uint8_t mac[6]) {
  SecureRecord record;
  return LoadRecord(role, &record) && memcmp(record.mac, mac, 6) == 0;
}
bool AcceptSecure(int role, const uint8_t mac[6], const uint8_t *service,
                  size_t size, uint8_t *plain, size_t *plain_size) {
  SecureRecord record;
  Envelope envelope;
  if (!LoadRecord(role, &record) || memcmp(record.mac, mac, 6) != 0 ||
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
