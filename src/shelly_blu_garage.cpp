#include "shelly_blu_garage.hpp"

#ifdef SHELLY_GARAGE_BLE
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <utility>
#include "mgos.hpp"
#include "mgos_bt_gap.h"
#include "mgos_sys_config.h"
#include "shelly_blu_bthome.hpp"
#include "shelly_blu_keys.hpp"
#include "shelly_blu_pairing.hpp"
#include "shelly_blu_xiaomi.hpp"
#include "shelly_blu_mibeacon.hpp"
#include "shelly_blu_xiaomi_gatt.hpp"

namespace shelly {
namespace blu {
namespace {
Sensor sensors[7];
mgos_bt_addr addresses[7];
bool configured = false;
ButtonEvents button_events;
ProgrammableButtonEvents programmable_button_events;
std::function<void(uint8_t)> button_event_handler;
bool button_pending = false;
double button_press_time = 0;
uint32_t button_press_sequence = 0;
uint32_t packet_sequence[7] = {};
bool known[7] = {};
int voltages[7] = {-1, -1, -1, -1, -1, -1, -1};

void RestoreContactReadings() {
  for (int i = 0; i < 2; ++i) {
    if (!known[i]) continue;
    bool active; int rssi; double age;
    if (CachedContact(i, addresses[i].addr, time(nullptr), mgos_uptime(), &active, &rssi, &age) && !sensors[i].seen) {
      sensors[i].seen = true; sensors[i].active = active;
      sensors[i].last_seen = mgos_uptime() - age; sensors[i].rssi = rssi;
    }
  }
}

void ScanResult(int, void *event_data, void *) {
  PairingAdvertisement(event_data);
  auto *result = static_cast<mgos_bt_gap_scan_result *>(event_data);
  int index = -1;
  for (int i = 0; i < 7; i++) {
    if (known[i] && memcmp(addresses[i].addr, result->addr.addr, 6) == 0)
      index = i;
  }
  if (index < 0) return;
  if (XiaomiBindkeyFor(index, result->addr.addr)) {
    const uint8_t *service = nullptr; size_t length = 0;
    if (!FindMiService(reinterpret_cast<const uint8_t *>(result->adv_data.p), result->adv_data.len, &service, &length) &&
        !FindMiService(reinterpret_cast<const uint8_t *>(result->scan_rsp.p), result->scan_rsp.len, &service, &length)) return;
    uint8_t plain[64]; size_t size=sizeof(plain); uint16_t model;
    Reading reading; bool contact;
    if (!AcceptMi(index,result->addr.addr,service,length,plain,&size,&model) ||
        !MiObjects(model,plain,size,&reading,&contact)) return;
    ApplyMiReading(index,result->addr,reading,contact,result->rssi);
    if (SensorReady(index)) PairingVerified(index);
    return;
  }
  if (QingpingFor(index,result->addr.addr)) {
    const uint8_t *service=nullptr;size_t size=0;Reading reading;bool state;
    if((!FindQingpingService(reinterpret_cast<const uint8_t *>(result->adv_data.p),result->adv_data.len,&service,&size) &&
        !FindQingpingService(reinterpret_cast<const uint8_t *>(result->scan_rsp.p),result->scan_rsp.len,&service,&size)) ||
       !AcceptQingping(index,result->addr.addr,service,size,&reading,&state))return;
    ApplyMiReading(index,result->addr,reading,state,result->rssi);
    if(SensorReady(index))PairingVerified(index);
    return;
  }
  if (XiaomiFor(index, result->addr.addr)) {
    Reading reading;
    if (!DecodeXiaomiAdvertisement(
            reinterpret_cast<const uint8_t *>(result->adv_data.p),
            result->adv_data.len, result->addr.addr, &reading) &&
        !DecodeXiaomiAdvertisement(
            reinterpret_cast<const uint8_t *>(result->scan_rsp.p),
            result->scan_rsp.len, result->addr.addr, &reading)) return;
    sensors[index].Update(reading, mgos_uptime(), result->rssi, 2);
    ++packet_sequence[index];
    if (SensorReady(index)) PairingVerified(index);
    return;
  }
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
  RestoreContactReadings();
  // Monitoring a securely paired sensor is independent of selecting BLU
  // as the garage's control inputs, including while only one role is paired.
  bool paired = false;
  for (int i = 0; i < 7; i++)
    paired = paired || SensorPaired(i);
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
  for (auto &voltage : voltages) voltage = -1;
  button_events = ButtonEvents();
  programmable_button_events = ProgrammableButtonEvents();
  button_pending = false;
  const char *macs[7] = {SensorMAC(0), SensorMAC(1), SensorMAC(2), SensorMAC(3), SensorMAC(4), SensorMAC(5), SensorMAC(6)};
  for (int i = 0; i < 7; i++) {
    known[i] = ValidMAC(macs[i]) &&
               mgos_bt_addr_from_str(mg_mk_str(macs[i]), &addresses[i]);
  }
  configured = known[0] && known[1] &&
               memcmp(addresses[0].addr, addresses[1].addr, 6) != 0;
  RestoreContactReadings();
}

void ForgetReading(int role) {
  if (role < 0 || role > 6) return;
  ForgetBattery(role);
  ForgetContact(role);
  sensors[role] = Sensor();
  voltages[role] = -1;
  packet_sequence[role] = 0;
  known[role] = false;
  addresses[role] = {};
  if (role < 2) configured = false;
  if (role == 5) {
    button_events = ButtonEvents();
    programmable_button_events = ProgrammableButtonEvents();
    button_pending = false;
    button_press_time = 0;
    button_press_sequence = 0;
  }
}

void ApplyMiReading(int role, const mgos_bt_addr &address, const Reading &reading,
                    bool contact, int rssi) {
  if (role<0 || (role>4 && role!=6)) return;
  if (!XiaomiBindkeyFor(role,address.addr) && !QingpingFor(role,address.addr)) return;
  addresses[role]=address; known[role]=true;
  configured=known[0] && known[1] && memcmp(addresses[0].addr,addresses[1].addr,6)!=0;
  if (role == 6 && reading.smoke < 0) {
    // Battery and test-button packets prove the beacon is alive, but they do
    // not establish whether smoke is present. Keep radio telemetry separate
    // from the smoke state so the panel can show RSSI without claiming clear.
    sensors[role].seen = true;
    sensors[role].last_seen = mgos_uptime();
    sensors[role].rssi = rssi;
    if (reading.battery >= 0) sensors[role].battery = reading.battery;
    if (reading.illuminance100 >= 0) {
      sensors[role].illuminance100 = reading.illuminance100;
      sensors[role].illuminance_last_seen = mgos_uptime();
    }
  } else if ((role>=3 && role<=4) || contact) {
    sensors[role].Update(reading,mgos_uptime(),rssi,role==6?3:role>=3?2:role==2?1:0);
    if (role == 6 && reading.smoke >= 0) {
      sensors[role].smoke_seen = true;
      sensors[role].smoke_last_seen = mgos_uptime();
    }
  }
  else {
    if(reading.battery>=0)sensors[role].battery=reading.battery;
    if(reading.illuminance100>=0){
      sensors[role].illuminance100=reading.illuminance100;
      sensors[role].illuminance_last_seen=mgos_uptime();
    }
  }
  if (contact && role < 2) CacheContact(role, address.addr, !reading.window_open, rssi, time(nullptr), mgos_uptime());
  if (reading.battery >= 0) CacheBattery(role, address.addr, reading.battery, time(nullptr), mgos_uptime());
  ++packet_sequence[role];
}

void Init() {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;
  ResetReadings();
  PairingInit();
  XiaomiGATTInit();
  mgos_event_add_handler(MGOS_BT_GAP_EVENT_SCAN_RESULT, ScanResult, nullptr);
  mgos_set_timer(1000, MGOS_TIMER_REPEAT, Scan, nullptr);
  Scan(nullptr);
}

bool Ready() {
  RestoreContactReadings();
  return configured && !PairingBusy() && (SecureFor(0, addresses[0].addr) || XiaomiBindkeyFor(0, addresses[0].addr) || QingpingFor(0, addresses[0].addr)) &&
         (SecureFor(1, addresses[1].addr) || XiaomiBindkeyFor(1, addresses[1].addr) || QingpingFor(1, addresses[1].addr)) &&
         mgos_bt_get_status() == MGOS_BT_STARTED &&
         (mgos_bt_gap_scan_in_progress() || XiaomiGATTScanPaused()) &&
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
    case 6: return mgos_sys_config_get_blu_smoke_mac();
    default: return "";
  }
}
const Sensor &MotionReading() { return sensors[2]; }
const char *SensorName(int role) {
  const char *name = nullptr;
  static const char *defaults[] = {"Closing sensor", "Opening sensor",
                                 "Motion sensor", "Indoor", "Outdoor", "Button", "Smoke sensor"};
  switch (role) {
    case 0: name = mgos_sys_config_get_blu_closed_name(); break;
    case 1: name = mgos_sys_config_get_blu_open_name(); break;
    case 2: name = mgos_sys_config_get_blu_motion_name(); break;
    case 3: name = mgos_sys_config_get_blu_indoor_name(); break;
    case 4: name = mgos_sys_config_get_blu_outdoor_name(); break;
    case 5: name = mgos_sys_config_get_blu_button_name(); break;
    case 6: name = mgos_sys_config_get_blu_smoke_name(); break;
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
    case 6: mgos_sys_config_set_blu_smoke_name(name); break;
  }
}
bool SensorDashboard(int role) {
  switch (role) {
    case 0: return mgos_sys_config_get_blu_closed_dashboard();
    case 1: return mgos_sys_config_get_blu_open_dashboard();
    case 2: return mgos_sys_config_get_blu_motion_dashboard();
    case 3: return mgos_sys_config_get_blu_indoor_dashboard();
    case 4: return mgos_sys_config_get_blu_outdoor_dashboard();
    case 5: return mgos_sys_config_get_blu_button_dashboard();
    case 6: return mgos_sys_config_get_blu_smoke_dashboard();
    default: return true;
  }
}
void SetSensorDashboard(int role, bool visible) {
  switch (role) {
    case 0: mgos_sys_config_set_blu_closed_dashboard(visible); break;
    case 1: mgos_sys_config_set_blu_open_dashboard(visible); break;
    case 2: mgos_sys_config_set_blu_motion_dashboard(visible); break;
    case 3: mgos_sys_config_set_blu_indoor_dashboard(visible); break;
    case 4: mgos_sys_config_set_blu_outdoor_dashboard(visible); break;
    case 5: mgos_sys_config_set_blu_button_dashboard(visible); break;
    case 6: mgos_sys_config_set_blu_smoke_dashboard(visible); break;
  }
}
const Sensor &ReadingFor(int role) { return sensors[role]; }
bool SensorPaired(int role) {
  mgos_bt_addr address;
  const char *mac = SensorMAC(role);
  return role >= 0 && role < 7 && ValidMAC(mac) &&
         mgos_bt_addr_from_str(mg_mk_str(mac), &address) &&
         (SecureFor(role, address.addr) || XiaomiBindkeyFor(role,address.addr) || QingpingFor(role,address.addr) || XiaomiFor(role, address.addr) ||
          XiaomiGATTFor(role, address.addr));
}
int SensorTimeout(int role) {
  // Thermometers may broadcast infrequently. Keep each measurement valid for
  // an hour, independently of the garage endpoints' shorter safety timeout.
  if (role == 3 || role == 4) return 3600;
  return mgos_sys_config_get_blu_stale_after_s();
}
void SetThermometerReading(int role, const mgos_bt_addr &address,
                           const Sensor &reading, int voltage_mv) {
  if (role != 3 && role != 4) return;
  mgos_bt_addr assigned;
  const char *assigned_mac = SensorMAC(role);
  if (!ValidMAC(assigned_mac) ||
      !mgos_bt_addr_from_str(mg_mk_str(assigned_mac), &assigned) ||
      memcmp(assigned.addr, address.addr, 6) != 0) return;
  addresses[role] = address;
  known[role] = true;
  sensors[role] = reading;
  voltages[role] = voltage_mv;
  ++packet_sequence[role];
}
bool SensorReady(int role) {
  if (role == 3 || role == 4) return HTReady(role, true) && HTReady(role, false);
  // A verified MiBeacon heartbeat/test/battery packet proves the detector is
  // reachable even when that packet does not carry object 0x1015 (smoke state).
  // Keep alarm state separate: only object 0x1015 may change SmokeDetected.
  if (role == 6) return SensorPaired(role) && sensors[role].Fresh(
         mgos_uptime(), mgos_sys_config_get_blu_stale_after_s()) &&
         mgos_bt_get_status() == MGOS_BT_STARTED;
  return role >= 0 && role < 7 && known[role] && (SecureFor(role, addresses[role].addr) || XiaomiBindkeyFor(role, addresses[role].addr) || QingpingFor(role, addresses[role].addr)) &&
         mgos_bt_get_status() == MGOS_BT_STARTED &&
         sensors[role].Fresh(mgos_uptime(), mgos_sys_config_get_blu_stale_after_s());
}
bool HTReady(int role, bool temperature) {
  return (role == 3 || role == 4) && SensorPaired(role) &&
         mgos_bt_get_status() == MGOS_BT_STARTED &&
         sensors[role].HTFresh(temperature, mgos_uptime(),
                              SensorTimeout(role));
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
         (mgos_bt_gap_scan_in_progress() || XiaomiGATTScanPaused()) &&
         sensors[5].Fresh(mgos_uptime(), mgos_sys_config_get_blu_presence_timeout_s());
}
bool CarPresent() {
  return CarPresenceReady();
}
bool MotionReady() { return SensorReady(2); }
bool MotionLightReady() {
  return MotionReady() && sensors[2].IlluminanceFresh(
                              mgos_uptime(), mgos_sys_config_get_blu_stale_after_s());
}
std::string GetInfoJSON() {
  RestoreContactReadings();
  std::string result = mgos::JSONPrintStringf(
      "{radio_enabled:%B,radio_running:%B,enable:%B,button_press_seq:%u,button_press_age_ms:%d,presence_timeout_s:%d,stale_after_s:%d,ready:%B", mgos_sys_config_get_bt_enable(), mgos_bt_get_status() == MGOS_BT_STARTED, Enabled(),
      button_press_sequence, button_press_sequence ? int((mgos_uptime() - button_press_time) * 1000) : -1,
      mgos_sys_config_get_blu_presence_timeout_s(),
      mgos_sys_config_get_blu_stale_after_s(), Ready());
  const char *button_name = mgos_sys_config_get_blu_button_name();
  const char *car_name = mgos_sys_config_get_blu_car_name();
  result += mgos::JSONPrintStringf(
      ",button_name:%Q,car_name:%Q,car_present:%B,car_presence_ready:%B",
      button_name ? button_name : "", car_name ? car_name : "", CarPresent(), CarPresenceReady());
  const char *roles[] = {"closed", "open", "motion", "indoor", "outdoor", "button", "smoke"};
  for (int i = 0; i < 7; i++) {
    const auto &sensor = sensors[i];
    const int battery = i < 2 && known[i] && XiaomiBindkeyFor(i, addresses[i].addr) && !QingpingFor(i, addresses[i].addr)
        ? CachedBattery(i, addresses[i].addr, time(nullptr), mgos_uptime()) : sensor.battery;
    result += mgos::JSONPrintStringf(
        ",%Q:%Q,%Q:{name:%Q,dashboard:%B,packet_seq:%u,paired:%B,protocol:%Q,ready:%B,temperature_ready:%B,humidity_ready:%B,secure:%B,seen:%B,smoke_seen:%B,smoke_age_s:%d,active:%B,battery:%d,rssi:%d,age_s:%d,temperature:%.1f,humidity:%d,illuminance:%.2f,illuminance_ready:%B,illuminance_age_s:%d,timeout_s:%d,battery_estimated:%B,voltage_mv:%d,gatt:%s}",
        (std::string(roles[i]) + "_mac").c_str(), SensorMAC(i) ? SensorMAC(i) : "", roles[i],
        SensorName(i), SensorDashboard(i), packet_sequence[i],
        SensorPaired(i), known[i] && QingpingFor(i,addresses[i].addr) ?
          (XiaomiBindkeyFor(i,addresses[i].addr)?"qingping_bindkey":"qingping") :
                         known[i] && XiaomiClockFor(i, addresses[i].addr) ?
                           (XiaomiGATTFor(i, addresses[i].addr) ? "xiaomi_clock_gatt" : "xiaomi_clock_bindkey") :
                         known[i] && XiaomiBindkeyFor(i, addresses[i].addr) ? "xiaomi_bindkey" :
                         known[i] && XiaomiGATTFor(i, addresses[i].addr) ? "xiaomi_gatt" :
                         known[i] && XiaomiFor(i, addresses[i].addr) ? "xiaomi" : "bthome",
        SensorReady(i), HTReady(i, true), HTReady(i, false),
        known[i] && (SecureFor(i, addresses[i].addr) || XiaomiBindkeyFor(i, addresses[i].addr)), sensor.seen,
        sensor.smoke_seen, sensor.smoke_seen ? int(mgos_uptime() - sensor.smoke_last_seen) : -1, sensor.active,
        battery, sensor.rssi, sensor.seen ? int(mgos_uptime() - sensor.last_seen) : -1,
        sensor.temperature10 / 10.0, sensor.humidity, sensor.illuminance100 / 100.0,
        SensorReady(i) && sensor.IlluminanceFresh(mgos_uptime(), SensorTimeout(i)),
        sensor.illuminance100 >= 0 ? int(mgos_uptime() - sensor.illuminance_last_seen) : -1,
        SensorTimeout(i), voltages[i] >= 0,
        voltages[i], XiaomiGATTStatusJSON(i).c_str());
  }
  return result + "}";
}
}  // namespace blu
}  // namespace shelly
#endif
