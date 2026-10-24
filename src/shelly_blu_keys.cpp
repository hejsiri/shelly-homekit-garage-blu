#include "shelly_blu_keys.hpp"
#ifdef SHELLY_GARAGE_BLE
#include <string.h>
#include "mbedtls/ccm.h"
#include "nvs.h"
#include "shelly_blu_secure.hpp"
#include "shelly_blu_xiaomi.hpp"

namespace shelly {
namespace blu {
namespace {
SecureRecord cache[6] = {};
bool loaded[6] = {};
bool ValidProtocol(int role, uint8_t protocol) {
  return protocol == kProtocolBTHome ? role <= 5 :
         protocol == kProtocolXiaomiContact ? (role == 0 || role == 1) :
         (protocol == kProtocolXiaomi || protocol == kProtocolXiaomiEncrypted) && (role == 3 || role == 4);
}
const char *Name(int role) {
  static const char *names[] = {"closed", "open", "motion", "indoor", "outdoor", "button"};
  return names[role];
}
}  // namespace
bool LoadRecord(int role, SecureRecord *record) {
  if (role < 0 || role > 5) return false;
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
  if (role < 0 || role > 5 || record.version != 1 ||
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
bool SecureFor(int role, const uint8_t mac[6]) {
  SecureRecord record;
  return LoadRecord(role, &record) && record.protocol == kProtocolBTHome &&
         memcmp(record.mac, mac, 6) == 0;
}
bool XiaomiFor(int role, const uint8_t mac[6]) {
  SecureRecord record;
  return LoadRecord(role, &record) && record.protocol != kProtocolBTHome &&
         memcmp(record.mac, mac, 6) == 0;
}
bool XiaomiSecureFor(int role, const uint8_t mac[6]) {
  SecureRecord record;
  return XiaomiFor(role, mac) && LoadRecord(role, &record) &&
         (record.protocol == kProtocolXiaomiEncrypted || record.protocol == kProtocolXiaomiContact);
}
bool XiaomiContactFor(int role, const uint8_t mac[6]) {
  SecureRecord record;
  return LoadRecord(role, &record) && record.protocol == kProtocolXiaomiContact &&
         memcmp(record.mac, mac, 6) == 0;
}
bool DecryptXiaomi(const uint8_t *data, size_t size, const uint8_t mac[6],
                   const uint8_t key[16], Reading *out, uint32_t *counter) {
  XiaomiFrame frame;
  if (!ParseXiaomiFrame(data, size, mac, &frame) || !frame.encrypted ||
      frame.length > 32) return false;
  uint8_t plain[32] = {};
  const uint8_t aad = 0x11;
  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);
  int result = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
  if (!result) result = mbedtls_ccm_auth_decrypt(
      &ctx, frame.length, frame.nonce, sizeof(frame.nonce), &aad, 1,
      frame.payload, plain, frame.mic, 4);
  mbedtls_ccm_free(&ctx);
  if (result || !DecodeXiaomiObjects(plain, frame.length, out, frame.model)) return false;
  out->packet_id = frame.packet_id;
  *counter = frame.counter;
  return true;
}
bool AcceptXiaomi(int role, const uint8_t mac[6], const uint8_t *data,
                  size_t size, Reading *out) {
  if (!XiaomiFor(role, mac)) return false;
  const uint8_t *service;
  size_t length;
  if (!XiaomiServiceData(data, size, &service, &length)) return false;
  SecureRecord record;
  if (!LoadRecord(role, &record)) return false;
  if (record.protocol == kProtocolXiaomi)
    return DecodeXiaomi(service, length, mac, out);
  XiaomiFrame frame;
  if (!ParseXiaomiFrame(service, length, mac, &frame) ||
      frame.model != (record.protocol == kProtocolXiaomiContact ? 0x098b : 0x055b)) return false;
  Reading reading;
  uint32_t counter;
  if (!DecryptXiaomi(service, length, mac, record.key, &reading, &counter) ||
      !NewCounter(record.counter_seen, record.counter, counter)) return false;
  record.counter_seen = true;
  record.counter = counter;
  if (!SaveRecord(role, record)) return false;
  *out = reading;
  return true;
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
