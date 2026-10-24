#include "shelly_blu_keys.hpp"
#include "shelly_blu_secure.hpp"
#include "shelly_blu_xiaomi.hpp"
#include "mbedtls/ccm.h"
#include "nvs.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>
using namespace shelly::blu;
static std::string path;
static std::vector<uint8_t> pending;
static bool fail_commit = false;
static std::string pending_name = "closed";
static std::string RecordPath(const char *name) {
  return std::string(name) == "closed" ? path : path + "." + name;
}
esp_err_t nvs_open(const char *, int, nvs_handle_t *h) { *h = 1; return 0; }
esp_err_t nvs_get_blob(nvs_handle_t, const char *name, void *data, size_t *length) {
  std::ifstream f(RecordPath(name), std::ios::binary);
  if (!f) return 1;
  f.read(static_cast<char *>(data), *length);
  return f.gcount() == static_cast<long>(*length) ? 0 : 1;
}
esp_err_t nvs_set_blob(nvs_handle_t, const char *name, const void *data, size_t n) {
  pending_name = name;
  auto p = static_cast<const uint8_t *>(data); pending.assign(p, p + n); return 0;
}
esp_err_t nvs_commit(nvs_handle_t) {
  if (fail_commit) return 1;
  std::ofstream f(RecordPath(pending_name.c_str()), std::ios::binary); f.write(reinterpret_cast<const char *>(pending.data()), pending.size());
  return f.good() ? 0 : 1;
}
void nvs_close(nvs_handle_t) {}
std::vector<uint8_t> Hex(const char *text) {
  std::vector<uint8_t> result;
  for (size_t i = 0; text[i]; i += 2) { unsigned value; sscanf(text + i, "%2x", &value); result.push_back(value); }
  return result;
}
std::vector<uint8_t> EncryptedXiaomi(const std::vector<uint8_t> &mac,
    const std::vector<uint8_t> &key,uint32_t counter,const std::vector<uint8_t> &objects) {
  auto data=Hex("58585b0500");data[4]=counter;
  data.insert(data.end(),mac.rbegin(),mac.rend());
  std::vector<uint8_t> nonce(mac.rbegin(),mac.rend());
  nonce.insert(nonce.end(),data.begin()+2,data.begin()+5);
  for(int i=1;i<=3;i++)nonce.push_back(counter>>(8*i));
  std::vector<uint8_t> cipher(objects.size()),mic(4);uint8_t aad=0x11;
  mbedtls_ccm_context ctx;mbedtls_ccm_init(&ctx);
  assert(!mbedtls_ccm_setkey(&ctx,MBEDTLS_CIPHER_ID_AES,key.data(),128));
  assert(!mbedtls_ccm_encrypt_and_tag(&ctx,objects.size(),nonce.data(),nonce.size(),&aad,1,
    objects.data(),cipher.data(),mic.data(),mic.size()));
  mbedtls_ccm_free(&ctx);data.insert(data.end(),cipher.begin(),cipher.end());
  for(int i=1;i<=3;i++)data.push_back(counter>>(8*i));
  data.insert(data.end(),mic.begin(),mic.end());return data;
}
std::vector<uint8_t> Advertisement(const std::vector<uint8_t> &data) {
 std::vector<uint8_t> adv={uint8_t(data.size()+3),0x16,0x95,0xfe};
 adv.insert(adv.end(),data.begin(),data.end());return adv;
}
int main(int argc, char **argv) {
  assert(argc >= 2); path = argv[1];
  // Published BTHome v2 AES-CCM interoperability vector (bthome.io/encryption).
  auto mac = Hex("5448e68f80a5");
  auto key = Hex("231d39c1d7cc1ab1aee224cd096db932");
  auto packet = Hex("d2fc41e445f3c9962b332211006c7c4519");
  uint8_t plain[32] = {}; size_t length = sizeof(plain);
  if (argc == 3) {
    assert(!AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
    assert(XiaomiFor(4,mac.data())&&!SecureFor(4,mac.data()));
    auto xm=Hex("a4c1380283f4"),xk=Hex("e9ea895fac7cca6d30532432a516f3a8");
    auto newer=Advertisement(EncryptedXiaomi(xm,xk,0x2653,Hex("041002f600")));
    Reading xr;assert(XiaomiSecureFor(3,xm.data()));
    assert(!AcceptXiaomi(3,xm.data(),newer.data(),newer.size(),&xr));
    puts("PASS: BTHome and Xiaomi replay protection survives process restart"); return 0;
  }
  SecureRecord record = {}; record.version = 1;
  memcpy(record.mac, mac.data(), 6); memcpy(record.key, key.data(), 16);
  assert(SaveRecord(0, record));
  auto bad = packet; bad.back() ^= 1;
  assert(!AcceptSecure(0, mac.data(), bad.data(), bad.size(), plain, &length));
  bad = packet; bad[3] ^= 1;
  assert(!AcceptSecure(0, mac.data(), bad.data(), bad.size(), plain, &length));
  bad = packet; bad[packet.size() - 8] ^= 1; // Counter is authenticated too.
  assert(!AcceptSecure(0, mac.data(), bad.data(), bad.size(), plain, &length));
  bad = packet; bad[2] &= ~1; // No plaintext downgrade.
  assert(!AcceptSecure(0, mac.data(), bad.data(), bad.size(), plain, &length));
  auto wrong_mac = mac; wrong_mac[0] ^= 1;
  assert(!AcceptSecure(0, wrong_mac.data(), packet.data(), packet.size(), plain, &length));
  for (size_t i = 0; i < packet.size(); i++) {
    assert(!AcceptSecure(0, mac.data(), packet.data(), i, plain, &length));
  }
  // A failure to persist freshness must never release authenticated state.
  fail_commit = true;
  assert(!AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
  fail_commit = false; length = sizeof(plain);
  assert(AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
  auto expected = Hex("4002ca0903bf13");
  assert(length == expected.size() && memcmp(plain, expected.data(), length) == 0);
  length = sizeof(plain);
  assert(!AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
  assert(!NewCounter(true, 100, 99) && !NewCounter(true, 100, 100));
  assert(NewCounter(true, 100, 101));
  assert(!NewCounter(true, UINT32_MAX, 0));
  // Motion has its own persistent key/counter slot; pairing it cannot reset
  // the already accepted garage packet's replay protection.
  assert(SaveRecord(2, record));
  length = sizeof(plain);
  assert(AcceptSecure(2, mac.data(), packet.data(), packet.size(), plain, &length));
  length = sizeof(plain);
  assert(!AcceptSecure(2, mac.data(), packet.data(), packet.size(), plain, &length));
  assert(!AcceptSecure(0, mac.data(), packet.data(), packet.size(), plain, &length));
    for (int role : {3,4}) {
    assert(SaveRecord(role, record));
    length = sizeof(plain);
    assert(AcceptSecure(role, mac.data(), packet.data(), packet.size(), plain, &length));
    length = sizeof(plain);
    assert(!AcceptSecure(role, mac.data(), packet.data(), packet.size(), plain, &length));
  }
  // A passive thermometer cannot authorize door, motion or button data.
  record.protocol=kProtocolXiaomi;
  for(int slot:{0,1,2,5})assert(!SaveRecord(slot,record));
  assert(SaveRecord(3,record)&&XiaomiFor(3,mac.data())&&!SecureFor(3,mac.data()));
  length=sizeof(plain);
  assert(!AcceptSecure(3,mac.data(),packet.data(),packet.size(),plain,&length));
  assert(!XiaomiFor(3,wrong_mac.data()));
  assert(SaveRecord(4,record));
  record.protocol=kProtocolBTHome;
  assert(SaveRecord(3,record)&&SecureFor(3,mac.data())&&!XiaomiFor(3,mac.data()));
  // Published stock LYWSD03MMC v5 vector: humidity rounded to 47 %, counter 0x2650.
  auto xm=Hex("a4c1380283f4"),xk=Hex("e9ea895fac7cca6d30532432a516f3a8");
  auto encrypted=Hex("58585b0550f4830238c1a495ef58763c26000097e2abb5");
  Reading xr;uint32_t counter=0;
  assert(DecryptXiaomi(encrypted.data(),encrypted.size(),xm.data(),xk.data(),&xr,&counter));
  assert(xr.humidity==47&&!xr.temperature_present&&counter==0x2650);
  SecureRecord xrecord={};xrecord.version=1;xrecord.protocol=kProtocolXiaomiEncrypted;
  memcpy(xrecord.mac,xm.data(),6);memcpy(xrecord.key,xk.data(),16);
  for(int slot:{0,1,2,5})assert(!SaveRecord(slot,xrecord));
  assert(SaveRecord(3,xrecord)&&XiaomiSecureFor(3,xm.data())&&!SecureFor(3,xm.data()));
  auto adv=Advertisement(encrypted);
  auto wrongkey=xk;wrongkey[0]^=1;
  assert(!DecryptXiaomi(encrypted.data(),encrypted.size(),xm.data(),wrongkey.data(),&xr,&counter));
  auto xbad=encrypted;xbad.back()^=1;
  assert(!DecryptXiaomi(xbad.data(),xbad.size(),xm.data(),xk.data(),&xr,&counter));
  for(size_t n=0;n<encrypted.size();n++)assert(!DecryptXiaomi(encrypted.data(),n,xm.data(),xk.data(),&xr,&counter));
  fail_commit=true;assert(!AcceptXiaomi(3,xm.data(),adv.data(),adv.size(),&xr));
  fail_commit=false;assert(AcceptXiaomi(3,xm.data(),adv.data(),adv.size(),&xr));
  assert(!AcceptXiaomi(3,xm.data(),adv.data(),adv.size(),&xr));
  assert(!AcceptXiaomi(0,xm.data(),adv.data(),adv.size(),&xr));
  // Independent authenticated temperature, battery and freshness. No downgrade.
  auto temp=Advertisement(EncryptedXiaomi(xm,xk,0x2651,Hex("041002ccff")));
  assert(AcceptXiaomi(3,xm.data(),temp.data(),temp.size(),&xr)&&xr.temperature10==-52);
  Sensor xs;xs.Update(xr,100,-60,2);assert(!xs.HTFresh(false,100,1800));
  auto batt=Advertisement(EncryptedXiaomi(xm,xk,0x2652,Hex("0a100150")));
  assert(AcceptXiaomi(3,xm.data(),batt.data(),batt.size(),&xr)&&xr.battery==80);
  xs.Update(xr,1901,-60,2);assert(!xs.HTFresh(true,1901,1800));
  auto invalid=Advertisement(EncryptedXiaomi(xm,xk,0x2653,Hex("041003010203")));
  assert(!AcceptXiaomi(3,xm.data(),invalid.data(),invalid.size(),&xr));
  auto newer=Advertisement(EncryptedXiaomi(xm,xk,0x2653,Hex("041002f600")));
  assert(AcceptXiaomi(3,xm.data(),newer.data(),newer.size(),&xr));
  auto wrongmac=xm;wrongmac[0]^=1;assert(!AcceptXiaomi(3,wrongmac.data(),newer.data(),newer.size(),&xr));
  auto plaintext=Hex("50305b0501f4830238c1a40d10041001ea01");
  auto downgrade=Advertisement(plaintext);assert(!AcceptXiaomi(3,xm.data(),downgrade.data(),downgrade.size(),&xr));
  puts("PASS: stock Xiaomi vector, authenticated objects, key/MAC validation, tampering, persistence, replay and downgrade rejection");
  assert(!SaveRecord(6, record));
  puts("PASS: independent Motion key and replay counter");
  puts("PASS: real AES-CCM vector, tampering, nonce identity, truncation, plaintext, persistence failure and replay");
}
