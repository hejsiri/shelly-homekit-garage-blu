#include "shelly_blu_keys.hpp"
#include "shelly_blu_secure.hpp"
#include "shelly_blu_mibeacon.hpp"
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
static bool erase_pending = false;
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
  erase_pending = false;
  pending_name = name;
  auto p = static_cast<const uint8_t *>(data); pending.assign(p, p + n); return 0;
}
esp_err_t nvs_erase_key(nvs_handle_t, const char *name) {
  pending_name = name; erase_pending = true;
  return 0;
}
esp_err_t nvs_commit(nvs_handle_t) {
  if (fail_commit) return 1;
  if (erase_pending) { std::remove(RecordPath(pending_name.c_str()).c_str()); erase_pending = false; return 0; }
  std::ofstream f(RecordPath(pending_name.c_str()), std::ios::binary); f.write(reinterpret_cast<const char *>(pending.data()), pending.size());
  return f.good() ? 0 : 1;
}
void nvs_close(nvs_handle_t) {}
std::vector<uint8_t> Hex(const char *text) {
  std::vector<uint8_t> result;
  for (size_t i = 0; text[i]; i += 2) { unsigned value; sscanf(text + i, "%2x", &value); result.push_back(value); }
  return result;
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
    SecureRecord deleted; assert(!LoadRecord(3, &deleted));
    auto mi_mac=Hex("e4aaec8121ae"), mi_packet=Hex("58588b09a3ae2181ecaae40e2c553c040000d28a500c");
    uint16_t mi_model; length=sizeof(plain);
    assert(!AcceptMi(1,mi_mac.data(),mi_packet.data(),mi_packet.size(),plain,&length,&mi_model));
    assert(XiaomiGATTFor(4,mac.data())&&!XiaomiFor(4,mac.data())&&!SecureFor(4,mac.data()));
    puts("PASS: replay rejected and Xiaomi binding retained after process restart"); return 0;
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
  record.protocol=kProtocolXiaomiGATT;
  for(int slot:{0,1,2,5})assert(!SaveRecord(slot,record));
  assert(SaveRecord(4,record)&&XiaomiGATTFor(4,mac.data())&&!XiaomiFor(4,mac.data())&&!SecureFor(4,mac.data()));
  assert(!XiaomiGATTFor(4,wrong_mac.data()));
  length=sizeof(plain);assert(!AcceptSecure(4,mac.data(),packet.data(),packet.size(),plain,&length));
  record.protocol=kProtocolClockGATT;
  assert(SaveRecord(4,record)&&XiaomiGATTFor(4,mac.data())&&XiaomiClockFor(4,mac.data()));
  assert(!XiaomiBindkeyFor(4,mac.data())&&!XiaomiClockFor(4,wrong_mac.data()));
  for(int slot:{0,1,2,5,6})assert(!SaveRecord(slot,record));
  // Removed encrypted-Xiaomi protocols cannot create bindings in any role.
  for(uint8_t protocol:{2,3}) for(int slot=0;slot<6;slot++) {
    auto removed=record;removed.protocol=protocol;assert(!SaveRecord(slot,removed));
  }
  // Delete only the selected key, retaining other roles and retrying failures.
  assert(!DeleteRecord(-1) && !DeleteRecord(7));
  fail_commit = true;
  assert(!DeleteRecord(3) && SecureFor(3, mac.data()));
  fail_commit = false;
  assert(DeleteRecord(3) && !LoadRecord(3, &record));
  assert(XiaomiGATTFor(4, mac.data()));
  assert(DeleteRecord(3));
  assert(!SaveRecord(6, record));
  // Public interoperability fixtures from Bluetooth-Devices/xiaomi-ble (MIT),
  // tests test_Xiaomi_MCCGQ02HL and test_Xiaomi_LYWSD03MMC_encrypted.
  // Public xiaomi-ble test_Xiaomi_XMPIRO2SXS fixture (MIT): 51 lx + motion.
  {
    SecureRecord motion={};motion.version=1;motion.protocol=kProtocolMiBindkey;
    auto addr=Hex("dc8e952dea43"),key=Hex("685d647dc5e7bc9bcfcf5a1357bd2114");
    auto beacon=Hex("485931350b64799117331ef4020000c5d2f6ac");
    memcpy(motion.mac,addr.data(),6);memcpy(motion.key,key.data(),16);
    uint16_t id;uint32_t seq;bool event;Reading r;size_t n=sizeof(plain);
    assert(DecryptMi(motion,beacon.data(),beacon.size(),plain,&n,&seq,&id));
    assert(id==0x3531&&MiObjects(id,plain,n,&r,&event)&&event&&r.motion&&r.illuminance100==5100);
    SecureRecord previous;assert(LoadRecord(2,&previous));
    assert(SaveRecord(2,motion));n=sizeof(plain);
    assert(AcceptMi(2,addr.data(),beacon.data(),beacon.size(),plain,&n,&id));
    n=sizeof(plain);assert(!AcceptMi(2,addr.data(),beacon.data(),beacon.size(),plain,&n,&id));
    assert(SaveRecord(2,previous));
    beacon.back()^=1;n=sizeof(plain);
    assert(!DecryptMi(motion,beacon.data(),beacon.size(),plain,&n,&seq,&id));
  }
  // Public xiaomi-ble test_Xiaomi_JTYJGD03MI_smoke fixture.
  {
    SecureRecord alarm={};alarm.version=1;alarm.protocol=kProtocolMiBindkey;
    auto addr=Hex("54ef44e39cbc"),key=Hex("5b51a7c91cde6707c9ef18dfda143a58");
    auto frame=Hex("5859970966bc9ce344ef5401081205000000715ebe90");
    memcpy(alarm.mac,addr.data(),6);memcpy(alarm.key,key.data(),16);
    uint16_t id;uint32_t seq;bool event;Reading r;size_t n=sizeof(plain);
    assert(DecryptMi(alarm,frame.data(),frame.size(),plain,&n,&seq,&id));
    assert(id==0x0997&&MiObjects(id,plain,n,&r,&event)&&event&&r.smoke==1);
    assert(MiRoleModel(6,id)&&!MiRoleModel(0,id)&&!MiRoleModel(2,id));
    assert(SaveRecord(6,alarm));n=sizeof(plain);
    assert(AcceptMi(6,addr.data(),frame.data(),frame.size(),plain,&n,&id));
    n=sizeof(plain);assert(!AcceptMi(6,addr.data(),frame.data(),frame.size(),plain,&n,&id));
    assert(DeleteRecord(6));frame.back()^=1;n=sizeof(plain);
    assert(!DecryptMi(alarm,frame.data(),frame.size(),plain,&n,&seq,&id));
    const uint8_t clear[]={0x15,0x10,1,0},battery[]={0x0a,0x10,1,77},button[]={1,0x10,3,0,0,1};
    assert(MiObjects(id,clear,sizeof(clear),&r,&event)&&event&&r.smoke==0);
    assert(MiObjects(id,battery,sizeof(battery),&r,&event)&&!event&&r.smoke==-1&&r.battery==77);
    assert(MiObjects(id,button,sizeof(button),&r,&event)&&!event&&r.smoke==-1);
    auto press=Hex("5859970964bc9ce344ef5422206088fd000000003a148fb3");n=sizeof(plain);
    assert(DecryptMi(alarm,press.data(),press.size(),plain,&n,&seq,&id));
    assert(MiObjects(id,plain,n,&r,&event)&&!event&&r.smoke==-1&&r.button>=0);
  }
  SecureRecord mi={};mi.version=1;mi.protocol=kProtocolMiBindkey;
  auto mmac=Hex("e4aaec8121ae"), mk=Hex("017e52d2684779298709b117c0a75a7b");
  auto mp=Hex("58588b09a3ae2181ecaae40e2c553c040000d28a500c");
  memcpy(mi.mac,mmac.data(),6);memcpy(mi.key,mk.data(),16);
  uint16_t model; uint32_t counter; bool contact; Reading reading;
  length=sizeof(plain);assert(DecryptMi(mi,mp.data(),mp.size(),plain,&length,&counter,&model));
  assert(model==0x098b && MiObjects(model,plain,length,&reading,&contact) && contact && reading.window_open);
  const uint8_t closed[]={0x19,0x10,1,1},open[]={0x19,0x10,1,0},invalid[]={0x19,0x10,1,3},battery[]={0x0a,0x10,1,90};
  assert(MiObjects(0x098b,closed,sizeof(closed),&reading,&contact)&&contact&&!reading.window_open);
  assert(MiObjects(0x098b,open,sizeof(open),&reading,&contact)&&contact&&reading.window_open);
  assert(!MiObjects(0x098b,invalid,sizeof(invalid),&reading,&contact));
  assert(MiObjects(0x098b,battery,sizeof(battery),&reading,&contact)&&!contact&&reading.battery==90);
  // MCCGQ02HL firmware also reports battery via object 0x4803 (ESPHome).
  for (int percent : {0, 57, 100}) {
    uint8_t alternate[]={0x03,0x48,1,static_cast<uint8_t>(percent)};
    assert(MiObjects(0x098b,alternate,sizeof(alternate),&reading,&contact));
    assert(!contact && reading.battery==percent);
    Sensor sensor; sensor.Update(reading,1,-60,0);
    assert(MiObjects(0x098b,closed,sizeof(closed),&reading,&contact));
    sensor.Update(reading,2,-60,0);
    assert(sensor.battery==percent && sensor.active);
  }
  const uint8_t invalid_battery[]={0x03,0x48,1,101};
  const uint8_t short_battery[]={0x03,0x48,1};
  const uint8_t duplicate_battery[]={0x0a,0x10,1,90,0x03,0x48,1,80};
  assert(!MiObjects(0x098b,invalid_battery,sizeof(invalid_battery),&reading,&contact));
  assert(!MiObjects(0x098b,short_battery,sizeof(short_battery),&reading,&contact));
  assert(!MiObjects(0x098b,duplicate_battery,sizeof(duplicate_battery),&reading,&contact));
  assert(SaveRecord(1,mi)&&XiaomiBindkeyFor(1,mmac.data()));
  length=sizeof(plain);assert(AcceptMi(1,mmac.data(),mp.data(),mp.size(),plain,&length,&model));
  length=sizeof(plain);assert(!AcceptMi(1,mmac.data(),mp.data(),mp.size(),plain,&length,&model));
  SecureRecord previous_motion;assert(LoadRecord(2,&previous_motion));
  assert(SaveRecord(2,mi)&&!SaveRecord(5,mi));
  length=sizeof(plain);assert(!AcceptMi(2,mmac.data(),mp.data(),mp.size(),plain,&length,&model));
  assert(SaveRecord(2,previous_motion));
  auto forged=mp;forged.back()^=1;length=sizeof(plain);
  assert(!DecryptMi(mi,forged.data(),forged.size(),plain,&length,&counter,&model));
  mi.key[0]^=1;length=sizeof(plain);assert(!DecryptMi(mi,mp.data(),mp.size(),plain,&length,&counter,&model));mi.key[0]^=1;
  for(size_t n=0;n<mp.size();++n){length=sizeof(plain);assert(!DecryptMi(mi,mp.data(),n,plain,&length,&counter,&model));}
  auto plain_ad=mp;plain_ad[0]&=~8;length=sizeof(plain);assert(!DecryptMi(mi,plain_ad.data(),plain_ad.size(),plain,&length,&counter,&model));
  mi.mac[0]^=1;length=sizeof(plain);assert(!DecryptMi(mi,mp.data(),mp.size(),plain,&length,&counter,&model));
  auto tm=Hex("a4c1380283f4"),tk=Hex("e9ea895fac7cca6d30532432a516f3a8");
  auto tp=Hex("58585b0550f4830238c1a495ef58763c26000097e2abb5");
  memcpy(mi.mac,tm.data(),6);memcpy(mi.key,tk.data(),16);
  length=sizeof(plain);assert(DecryptMi(mi,tp.data(),tp.size(),plain,&length,&counter,&model));
  assert(model==0x055b&&MiObjects(model,plain,length,&reading,&contact)&&!contact&&reading.humidity==47);
  assert(SaveRecord(3,mi));length=sizeof(plain);assert(AcceptMi(3,tm.data(),tp.data(),tp.size(),plain,&length,&model));
  fail_commit=true; // Persistence failures must never expose readings.
  assert(!SaveRecord(3,mi));fail_commit=false;
  assert(DeleteRecord(3)&&!XiaomiBindkeyFor(3,tm.data()));
  puts("PASS: authenticated Xiaomi contact/thermometer fixtures, MIC/key/MAC failures, truncation, protocol isolation, replay and bindkey deletion");
  {SecureRecord saved;assert(LoadRecord(1,&saved));
  SecureRecord q={};q.version=1;q.protocol=kProtocolQingpingBindkey;
  auto qm=Hex("58342d010203"),qk=Hex("000102030405060708090a0b0c0d0e0f"),qp=Hex("5858d603010302012d3458504138e80000007a93b39c");
  memcpy(q.mac,qm.data(),6);memcpy(q.key,qk.data(),16);assert(SaveRecord(1,q));
  length=sizeof(plain);assert(AcceptMi(1,qm.data(),qp.data(),qp.size(),plain,&length,&model));
  assert(MiObjects(model,plain,length,&reading,&contact)&&contact);
  length=sizeof(plain);assert(!AcceptMi(1,qm.data(),qp.data(),qp.size(),plain,&length,&model));
  assert(SaveRecord(1,saved));}
  {SecureRecord saved;assert(LoadRecord(2,&saved));
  SecureRecord q={};q.version=1;q.protocol=kProtocolQingpingBindkey;
  auto qm=Hex("58342d010203"),qk=Hex("000102030405060708090a0b0c0d0e0f"),qp=Hex("5858830a010302012d34580855b69d586d00000005c50e28");
  memcpy(q.mac,qm.data(),6);memcpy(q.key,qk.data(),16);assert(SaveRecord(2,q));
  length=sizeof(plain);assert(AcceptMi(2,qm.data(),qp.data(),qp.size(),plain,&length,&model));
  assert(MiObjects(model,plain,length,&reading,&contact)&&contact);
  length=sizeof(plain);assert(!AcceptMi(2,qm.data(),qp.data(),qp.size(),plain,&length,&model));
  assert(SaveRecord(2,saved));}
  {SecureRecord q={};q.version=1;q.protocol=kProtocolQingpingBindkey;
  auto qm=Hex("5a582d34125f"),qk=Hex("814aac74c4f17b6c1581e1ab87816b99"),qp=Hex("5858480b685f12342d585a0b1841e2aa000e00a4964fb5");
  memcpy(q.mac,qm.data(),6);memcpy(q.key,qk.data(),16);assert(SaveRecord(3,q));
  length=sizeof(plain);assert(AcceptMi(3,qm.data(),qp.data(),qp.size(),plain,&length,&model));
  assert(model==0x0b48&&MiObjects(model,plain,length,&reading,&contact)&&reading.humidity==60&&!contact);
  assert(DeleteRecord(3));}
  puts("PASS: Qingping CGG1 interoperability, authenticated CGH1/CGPR1, replay rejection and isolated records");
  puts("PASS: independent Motion key and replay counter");
  puts("PASS: real AES-CCM vector, tampering, nonce identity, truncation, plaintext, persistence failure and replay");
}
