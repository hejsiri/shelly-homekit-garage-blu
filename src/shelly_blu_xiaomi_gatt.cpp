#include "shelly_blu_xiaomi_gatt.hpp"
#ifdef SHELLY_GARAGE_BLE
#include <atomic>
#include <string.h>
#include <utility>
#include <vector>
#include "esp32xx_bt.h"
#include "mgos.hpp"
#include "mgos_bt_gap.h"
#include "shelly_blu_garage.hpp"
#include "shelly_blu_keys.hpp"
#include "shelly_blu_pairing.hpp"
extern "C" {
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
}
namespace shelly {
namespace blu {
namespace {
// One short, read-only connection at a time. Never starts SMP or writes GATT.
std::atomic<unsigned> ticket(0);
bool busy = false, connecting = false, moving = false;
uint16_t connection = BLE_HS_CONN_HANDLE_NONE;
int selected = -1, step = 0, signal = 0;
unsigned session = 0;
double expires = 0, cooldown = 5, scan_grace = 0;
XiaomiGATTCallback completion;
std::vector<uint8_t> value;
bool duplicate = false;
unsigned matches = 0;
struct Status {
  double next = 5;
  unsigned failures = 0;
  std::string firmware, hardware, error;
} status[2];
const char *uuids[] = {
  "2a24",  // Model, mandatory identity check.
  "2a26",
  "2a27",
  "ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6",
  "8edfffef-3d1b-9c37-4623-ad7265f14076",  // Stock mirror, if primary absent.
};
void Finish(const Sensor &reading, int voltage, const std::string &error) {
  if (!busy) return;
  ticket++;
  busy = false;
  if (connecting) ble_gap_conn_cancel();
  connecting = false;
  const uint16_t old = connection;
  connection = BLE_HS_CONN_HANDLE_NONE;
  if (old != BLE_HS_CONN_HANDLE_NONE) ble_gap_terminate(old, BLE_ERR_REM_USER_CONN_TERM);
  auto &s = status[selected - 3];
  s.error = error;
  s.failures = error.empty() ? 0 : s.failures + 1;
  s.next = mgos_uptime() + (s.failures == 1 ? 60 : s.failures == 2 ? 120 : 300);
  cooldown = mgos_uptime() + 10;
  auto callback = std::move(completion);
  completion = nullptr;
  if (callback) callback(reading, voltage, error);
}
void Fail(const std::string &error) { Finish(Sensor(), -1, error); }
void Read();
void ReadFinished(int result) {
  if (step == 0) {
    const std::string model(value.begin(), value.end());
    if (result || duplicate || model != "LYWSD03MMC") {
      Fail(mgos::SPrintf("Cannot verify LYWSD03MMC model (GATT 0x%x); close other Bluetooth apps and retry", result));
      return;
    }
  } else if (step <= 2) {
    // Version strings are diagnostic only. A missing optional field is allowed.
    if (!result && !duplicate && value.size() <= 24) {
      std::string text(value.begin(), value.end());
      for (char &c : text) if (uint8_t(c) < 32 || uint8_t(c) > 126) c = '?';
      (step == 1 ? status[selected - 3].firmware : status[selected - 3].hardware) = text;
    }
  } else {
    Reading decoded;
    int voltage = -1;
    if (!result && !duplicate && DecodeXiaomi03GATT(value.data(), value.size(), &decoded, &voltage)) {
      Sensor reading;
      reading.Update(decoded, mgos_uptime(), signal, 2);
      Finish(reading, voltage, "");
      return;
    }
    if (step == 4) {
      Fail(mgos::SPrintf("LYWSD03MMC direct read failed (GATT 0x%x, length %d); no key or PIN was requested", result, int(value.size())));
      return;
    }
  }
  step++;
  Read();
}
int ReadResult(uint16_t handle, const ble_gatt_error *error,
               ble_gatt_attr *attr, void *arg) {
  const unsigned current = uintptr_t(arg);
  const int result = error->status;
  std::vector<uint8_t> bytes;
  bool valid = true;
  if (!result) {
    const int size = attr && attr->om ? OS_MBUF_PKTLEN(attr->om) : -1;
    if (size < 0 || size > 32) valid = false;
    else {
      bytes.resize(size);
      if (size && os_mbuf_copydata(attr->om, 0, size, bytes.data())) valid = false;
    }
  }
  mgos::InvokeCB([current, handle, result, bytes, valid] {
    if (!busy || current != ticket || handle != connection) return;
    if (!result) {
      duplicate = duplicate || ++matches > 1 || !valid;
      value = bytes;
    } else {
      // Read-by-UUID can return multiple matches; advance only on completion.
      ReadFinished(result == BLE_HS_EDONE ? 0 : result);
    }
  });
  return 0;
}
void Read() {
  value.clear();
  duplicate = false;
  matches = 0;
  mgos_bt_uuid uuid;
  ble_uuid_any_t native;
  mgos_bt_uuid_from_str(mg_mk_str(uuids[step]), &uuid);
  mgos_bt_uuid_to_esp32(&uuid, &native);
  const unsigned current = ++ticket;
  // Search the full ATT range by UUID: handles vary with Xiaomi firmware.
  const int result = ble_gattc_read_by_uuid(connection, 1, 0xffff, &native.u,
      ReadResult, reinterpret_cast<void *>(uintptr_t(current)));
  if (result) ReadFinished(result);
}
int GapEvent(ble_gap_event *event, void *arg) {
  // Connection ticket stays distinct from each subsequent GATT read ticket.
  const unsigned current = uintptr_t(arg);
  if (event->type == BLE_GAP_EVENT_CONNECT) {
    const int result = event->connect.status;
    const uint16_t handle = event->connect.conn_handle;
    mgos::InvokeCB([current, result, handle] {
      if (!busy || !connecting || current != ticket) {
        if (!result) ble_gap_terminate(handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
      }
      connecting = false;
      if (result) { Fail(mgos::SPrintf("LYWSD03MMC connection failed (0x%x); close other Bluetooth apps and retry", result)); return; }
      connection = handle;
      int8_t rssi = 0;
      if (!ble_gap_conn_rssi(handle, &rssi)) signal = rssi;
      step = 0;
      Read();
    });
  } else if (event->type == BLE_GAP_EVENT_DISCONNECT) {
    const uint16_t handle = event->disconnect.conn.conn_handle;
    const int reason = event->disconnect.reason;
    mgos::InvokeCB([current, handle, reason] {
      if (busy && current == session && handle == connection)
        Fail(mgos::SPrintf("LYWSD03MMC disconnected (0x%x); close other Bluetooth apps and retry", reason));
    });
  }
  return 0;
}
void Tick(void *) {
  const double now = mgos_uptime();
  if (busy) {
    if (now >= expires) Fail("LYWSD03MMC read timed out; close other Bluetooth apps and keep it near Shelly");
    return;
  }
  if (moving || PairingBusy() || PairingScan() || now < cooldown || mgos_bt_get_status() != MGOS_BT_STARTED) return;
  for (int role = 3; role <= 4; role++) {
    mgos_bt_addr peer;
    if (now < status[role - 3].next || !ValidMAC(SensorMAC(role)) ||
        !mgos_bt_addr_from_str(mg_mk_str(SensorMAC(role)), &peer) ||
        !XiaomiGATTFor(role, peer.addr)) continue;
    const auto address = peer;
    XiaomiGATTStart(role, peer, [role, address](const Sensor &reading, int voltage, const std::string &error) {
      if (error.empty() && XiaomiGATTFor(role, address.addr))
        SetThermometerReading(role, address, reading, voltage);
    });
    break;
  }
}
}  // namespace
bool XiaomiGATTStart(int role, const mgos_bt_addr &peer, XiaomiGATTCallback callback) {
  if (busy || moving || (role != 3 && role != 4)) return false;
  busy = connecting = true;
  selected = role;
  completion = std::move(callback);
  expires = mgos_uptime() + 10;
  status[role - 3].firmware.clear();
  status[role - 3].hardware.clear();
  status[role - 3].error.clear();
  const unsigned current = ++ticket;
  ble_addr_t native;
  session = current;
  scan_grace = mgos_uptime() + 4;
  mgos_bt_addr_to_esp32(&peer, &native);
  mgos_bt_gap_scan_stop();
  uint8_t own_type;
  if (ble_hs_id_infer_auto(0, &own_type)) { Fail("Cannot select BLE address"); return true; }
  const int result = ble_gap_connect(own_type, &native, 3000, nullptr, GapEvent,
      reinterpret_cast<void *>(uintptr_t(current)));
  if (result) Fail(mgos::SPrintf("Cannot connect to LYWSD03MMC (0x%x)", result));
  return true;
}
void XiaomiGATTCancel() {
  // Cancel does not deliver a measurement or a completion callback.
  completion = nullptr;
  if (busy) Fail("Read cancelled");
}
bool XiaomiGATTConnecting() { return connecting; }
bool XiaomiGATTScanPaused() { return mgos_uptime() < scan_grace; }
void XiaomiGATTSetGarageMoving(bool value) {
  moving = value;
  if (moving && busy) Fail("Read interrupted while the garage is moving; retry when stopped");
}
std::string XiaomiGATTStatusJSON(int role) {
  if (role != 3 && role != 4) return "{}";
  const auto &s = status[role - 3];
  return mgos::JSONPrintStringf("{firmware:%Q,hardware:%Q,error:%Q,poll_interval_s:300}",
      s.firmware.c_str(), s.hardware.c_str(), s.error.c_str());
}
void XiaomiGATTInit() { mgos_set_timer(500, MGOS_TIMER_REPEAT, Tick, nullptr); }
}  // namespace blu
}  // namespace shelly
#endif
