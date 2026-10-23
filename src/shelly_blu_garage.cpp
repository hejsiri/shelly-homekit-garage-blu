#include "shelly_blu_garage.hpp"

#ifdef SHELLY_GARAGE_BLE
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include "mgos.hpp"
#include "mgos_bt_gap.h"
#include "mgos_sys_config.h"
#include "shelly_blu_bthome.hpp"
#include "shelly_blu_keys.hpp"
#include "shelly_blu_pairing.hpp"

namespace shelly {
namespace blu {
namespace {
Sensor sensors[2];
mgos_bt_addr addresses[2];
bool configured = false;
bool known[2] = {};

void ScanResult(int, void *event_data, void *) {
  PairingAdvertisement(event_data);
  auto *result = static_cast<mgos_bt_gap_scan_result *>(event_data);
  int index = -1;
  for (int i = 0; i < 2; i++) {
    if (known[i] && memcmp(addresses[i].addr, result->addr.addr, 6) == 0)
      index = i;
  }
  if (index < 0) return;
  const auto *data = reinterpret_cast<const uint8_t *>(result->adv_data.p);
  size_t length = result->adv_data.len;
  const uint8_t *service = nullptr;
  size_t service_size = 0;
  for (size_t i = 0; i < length;) {
    size_t n = data[i++];
    if (n == 0) break;
    if (n > length - i) return;
    if (n >= 3 && data[i] == 0x16 && data[i + 1] == 0xd2 &&
        data[i + 2] == 0xfc) {
      if (service != nullptr) return;
      service = data + i + 1;
      service_size = n - 1;
    }
    i += n;
  }
  uint8_t plain[32];
  size_t plain_size = sizeof(plain);
  Reading reading;
  // Once paired, plaintext can never downgrade a sensor to an insecure mode.
  if (service == nullptr ||
      !AcceptSecure(index, result->addr.addr, service, service_size, plain,
                    &plain_size) ||
      !Decode(plain, plain_size, &reading))
    return;
  sensors[index].Update(reading, mgos_uptime(), result->rssi);
  PairingVerified(index);
}

void Scan(void *) {
  if ((!Enabled() && !PairingScan()) || PairingConnecting() ||
      mgos_bt_get_status() != MGOS_BT_STARTED || mgos_bt_gap_scan_in_progress())
    return;
  mgos_bt_gap_scan_opts opts = {};
  opts.duration_ms = MGOS_BT_GAP_SCAN_DURATION_INFINITE;
  opts.active = PairingScan();
  opts.interval_ms = 150;
  opts.window_ms = 100;
  if (!mgos_bt_gap_scan(&opts))
    LOG(LL_ERROR, ("BLU garage: scan start failed"));
}
}  // namespace

bool ValidMAC(const char *mac) {
  if (mac == nullptr || strlen(mac) != 17) return false;
  bool nonzero = false;
  for (int i = 0; i < 17; i++) {
    if (i % 3 == 2) {
      if (mac[i] != ':') return false;
    } else {
      if (!isxdigit(static_cast<unsigned char>(mac[i]))) return false;
      nonzero = nonzero || mac[i] != '0';
    }
  }
  return nonzero;
}

void Invalidate() {
  configured = false;
}

bool Enabled() {
  return mgos_sys_config_get_blu_enable();
}

void ResetReadings() {
  sensors[0] = Sensor();
  sensors[1] = Sensor();
  const char *macs[2] = {mgos_sys_config_get_blu_closed_mac(),
                         mgos_sys_config_get_blu_open_mac()};
  for (int i = 0; i < 2; i++) {
    known[i] = ValidMAC(macs[i]) &&
               mgos_bt_addr_from_str(mg_mk_str(macs[i]), &addresses[i]);
  }
  configured = known[0] && known[1] &&
               memcmp(addresses[0].addr, addresses[1].addr, 6) != 0;
}

void Init() {
  ResetReadings();
  PairingInit();
  mgos_event_add_handler(MGOS_BT_GAP_EVENT_SCAN_RESULT, ScanResult, nullptr);
  mgos_set_timer(1000, MGOS_TIMER_REPEAT, Scan, nullptr);
  Scan(nullptr);
}

bool Ready() {
  return configured && !PairingBusy() && SecureFor(0, addresses[0].addr) &&
         SecureFor(1, addresses[1].addr) &&
         mgos_bt_get_status() == MGOS_BT_STARTED &&
         mgos_bt_gap_scan_in_progress() &&
         blu::Ready(sensors[0], sensors[1], mgos_uptime(),
                    mgos_sys_config_get_blu_stale_after_s());
}

void GetInputs(int *closed, int *open) {
  *closed = sensors[0].active;
  *open = sensors[1].active;
}

std::string GetInfoJSON() {
  double now = mgos_uptime();
  const char *closed = mgos_sys_config_get_blu_closed_mac();
  const char *open = mgos_sys_config_get_blu_open_mac();
  return mgos::JSONPrintStringf(
      "{enable:%B,closed_mac:%Q,open_mac:%Q,stale_after_s:%d,ready:%B,"
      "closed:{secure:%B,seen:%B,active:%B,battery:%d,rssi:%d,age_s:%d},"
      "open:{secure:%B,seen:%B,active:%B,battery:%d,rssi:%d,age_s:%d}}",
      Enabled(), closed ? closed : "", open ? open : "",
      mgos_sys_config_get_blu_stale_after_s(), Ready(),
      known[0] && SecureFor(0, addresses[0].addr), sensors[0].seen,
      sensors[0].active, sensors[0].battery, sensors[0].rssi,
      sensors[0].seen ? int(now - sensors[0].last_seen) : -1,
      known[1] && SecureFor(1, addresses[1].addr), sensors[1].seen,
      sensors[1].active, sensors[1].battery, sensors[1].rssi,
      sensors[1].seen ? int(now - sensors[1].last_seen) : -1);
}
}  // namespace blu
}  // namespace shelly
#endif
