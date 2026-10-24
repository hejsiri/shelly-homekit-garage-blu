#include "shelly_blu_garage.hpp"

#ifdef SHELLY_GARAGE_BLE
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <utility>
#include "mgos.hpp"
#include "mgos_bt_gap.h"
#include "mgos_sys_config.h"
#include "shelly_blu_bthome.hpp"
#include "shelly_blu_keys.hpp"
#include "shelly_blu_pairing.hpp"

namespace shelly {
namespace blu {
namespace {
Sensor sensors[6];
mgos_bt_addr addresses[6];
bool configured = false;
ButtonEvents button_events;
ProgrammableButtonEvents programmable_button_events;
std::function<void(uint8_t)> button_event_handler;
bool button_pending = false;
double button_press_time = 0;
uint32_t button_press_sequence = 0;
uint32_t packet_sequence[6] = {};
bool known[6] = {};

void ScanResult(int, void *event_data, void *) {
  PairingAdvertisement(event_data);
  auto *result = static_cast<mgos_bt_gap_scan_result *>(event_data);
  int index = -1;
  for (int i = 0; i < 6; i++) {
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
      !(index == 5 ? DecodeButton(plain, plain_size, &reading) : index >= 3 ? DecodeHT(plain, plain_size, &reading) :
                       Decode(plain, plain_size, &reading, index == 2)))
    return;
  const bool button_event = index == 5 && button_events.Accept(reading, mgos_uptime());
  const int switch_event = index == 5 ? programmable_button_events.Accept(reading) : -1;
  const bool events_allowed = !PairingBusy();
  if (button_event && events_allowed) {
    ++button_press_sequence;
    button_pending = true;
    button_press_time = mgos_uptime();
  }
  sensors[index].Update(reading, mgos_uptime(), result->rssi, index == 2 ? 1 : index >= 3 ? 2 : 0);
  ++packet_sequence[index];
  if (switch_event >= 0 && events_allowed && button_event_handler)
    button_event_handler(static_cast<uint8_t>(switch_event));
  PairingVerified(index);
}

void Scan(void *) {
  // Monitoring a securely paired sensor is independent of selecting BLU
  // as the garage's control inputs, including while only one role is paired.
  bool paired = false;
  for (int i = 0; i < 6; i++)
    paired = paired || (known[i] && SecureFor(i, addresses[i].addr));
  if ((!Enabled() && !paired && !PairingScan()) || PairingConnecting() ||
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
  ResetReadings();
}

bool Enabled() {
  return mgos_sys_config_get_blu_enable();
}

void ResetReadings() {
  for (auto &sensor : sensors) sensor = Sensor();
  button_events = ButtonEvents();
  programmable_button_events = ProgrammableButtonEvents();
  button_pending = false;
  const char *macs[6] = {SensorMAC(0), SensorMAC(1), SensorMAC(2), SensorMAC(3), SensorMAC(4), SensorMAC(5)};
  for (int i = 0; i < 6; i++) {
    known[i] = ValidMAC(macs[i]) &&
               mgos_bt_addr_from_str(mg_mk_str(macs[i]), &addresses[i]);
  }
  configured = known[0] && known[1] &&
               memcmp(addresses[0].addr, addresses[1].addr, 6) != 0;
}

void Init() {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;
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

const char *SensorMAC(int role) {
  switch (role) {
    case 0: return mgos_sys_config_get_blu_closed_mac();
    case 1: return mgos_sys_config_get_blu_open_mac();
    case 2: return mgos_sys_config_get_blu_motion_mac();
    case 3: return mgos_sys_config_get_blu_indoor_mac();
    case 4: return mgos_sys_config_get_blu_outdoor_mac();
    case 5: return mgos_sys_config_get_blu_button_mac();
    default: return "";
  }
}
const Sensor &MotionReading() { return sensors[2]; }
const char *SensorName(int role) {
  const char *name = nullptr;
  static const char *defaults[] = {"Closing sensor", "Opening sensor",
                                 "Motion sensor", "Indoor", "Outdoor", "Button"};
  switch (role) {
    case 0: name = mgos_sys_config_get_blu_closed_name(); break;
    case 1: name = mgos_sys_config_get_blu_open_name(); break;
    case 2: name = mgos_sys_config_get_blu_motion_name(); break;
    case 3: name = mgos_sys_config_get_blu_indoor_name(); break;
    case 4: name = mgos_sys_config_get_blu_outdoor_name(); break;
    case 5: name = mgos_sys_config_get_blu_button_name(); break;
    default: return "";
  }
  return name && *name ? name : defaults[role];
}
void SetSensorName(int role, const char *name) {
  switch (role) {
    case 0: mgos_sys_config_set_blu_closed_name(name); break;
    case 1: mgos_sys_config_set_blu_open_name(name); break;
    case 2: mgos_sys_config_set_blu_motion_name(name); break;
    case 3: mgos_sys_config_set_blu_indoor_name(name); break;
    case 4: mgos_sys_config_set_blu_outdoor_name(name); break;
    case 5: mgos_sys_config_set_blu_button_name(name); break;
  }
}
const Sensor &ReadingFor(int role) { return sensors[role]; }
bool SensorPaired(int role) {
  mgos_bt_addr address;
  const char *mac = SensorMAC(role);
  return role >= 0 && role < 6 && ValidMAC(mac) &&
         mgos_bt_addr_from_str(mg_mk_str(mac), &address) &&
         SecureFor(role, address.addr);
}
bool SensorReady(int role) {
  return role >= 0 && role < 6 && known[role] && SecureFor(role, addresses[role].addr) &&
         mgos_bt_get_status() == MGOS_BT_STARTED &&
         sensors[role].Fresh(mgos_uptime(), mgos_sys_config_get_blu_stale_after_s());
}
bool ConsumeButtonPress() {
  bool result = button_pending && mgos_uptime() - button_press_time <= 1;
  button_pending = false;
  return result;
}
void SetButtonEventHandler(std::function<void(uint8_t)> handler) {
  button_event_handler = std::move(handler);
}
const char *CarName() {
  const char *name = mgos_sys_config_get_blu_car_name();
  return name && *name ? name : "Car presence";
}
bool CarPresenceReady() {
  return SensorPaired(5) && mgos_bt_get_status() == MGOS_BT_STARTED &&
         mgos_bt_gap_scan_in_progress();
}
bool CarPresent() {
  return CarPresenceReady() &&
         sensors[5].Fresh(mgos_uptime(), mgos_sys_config_get_blu_presence_timeout_s());
}
bool MotionReady() { return SensorReady(2); }
bool MotionLightReady() {
  return MotionReady() && sensors[2].IlluminanceFresh(
                              mgos_uptime(), mgos_sys_config_get_blu_stale_after_s());
}
std::string GetInfoJSON() {
  std::string result = mgos::JSONPrintStringf(
      "{enable:%B,button_press_seq:%u,button_press_age_ms:%d,presence_timeout_s:%d,stale_after_s:%d,ready:%B", Enabled(),
      button_press_sequence, button_press_sequence ? int((mgos_uptime() - button_press_time) * 1000) : -1,
      mgos_sys_config_get_blu_presence_timeout_s(),
      mgos_sys_config_get_blu_stale_after_s(), Ready());
  const char *button_name = mgos_sys_config_get_blu_button_name();
  const char *car_name = mgos_sys_config_get_blu_car_name();
  result += mgos::JSONPrintStringf(
      ",button_name:%Q,car_name:%Q,car_present:%B,car_presence_ready:%B",
      button_name ? button_name : "", car_name ? car_name : "", CarPresent(), CarPresenceReady());
  const char *roles[] = {"closed", "open", "motion", "indoor", "outdoor", "button"};
  for (int i = 0; i < 6; i++) {
    const auto &sensor = sensors[i];
    result += mgos::JSONPrintStringf(
        ",%Q:%Q,%Q:{name:%Q,packet_seq:%u,secure:%B,seen:%B,active:%B,battery:%d,rssi:%d,age_s:%d,temperature:%.1f,humidity:%d}",
        (std::string(roles[i]) + "_mac").c_str(), SensorMAC(i) ? SensorMAC(i) : "", roles[i],
        SensorName(i), packet_sequence[i],
        known[i] && SecureFor(i, addresses[i].addr), sensor.seen, sensor.active,
        sensor.battery, sensor.rssi, sensor.seen ? int(mgos_uptime() - sensor.last_seen) : -1,
        sensor.temperature10 / 10.0, sensor.humidity);
  }
  return result + "}";
}
}  // namespace blu
}  // namespace shelly
#endif
