#include "shelly_blu_pairing.hpp"
#ifdef SHELLY_GARAGE_BLE
#include <string.h>
#include <array>
#include <atomic>
#include <string>
#include <vector>
#include "esp32xx_bt.h"
#include "mgos.hpp"
#include "mgos_bt_gap.h"
#include "mgos_rpc.h"
#include "mgos_sys_config.h"
extern "C" {
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "host/util/util.h"
}
#include "shelly_blu_garage.hpp"
#include "shelly_blu_keys.hpp"

namespace shelly {
namespace blu {
namespace {
struct Candidate {
  mgos_bt_addr addr;
  int rssi;
  bool encrypted;
  double seen;
};
std::vector<Candidate> candidates;
std::string state = "idle", error;
double deadline = 0;
int role = -1;
mgos_bt_addr peer = {};
ble_addr_t native_peer = {};
std::atomic<uint32_t> passkey(0);
uint32_t new_pin = 0;
bool rotated = false;
uint16_t conn = BLE_HS_CONN_HANDLE_NONE;
uint16_t svc_start = 0, svc_end = 0, pin_handle = 0, key_handle = 0,
         beacon_handle = 0;
bool existing = false, reconnecting = false;
uint8_t saved_io = 0, saved_bonding = 0, saved_mitm = 0;
bool security_changed = false;
std::atomic<unsigned> generation(0);
std::atomic<bool> repeat_used(false);

bool Active() {
  return state != "idle" && state != "done" && state != "error";
}
void RestoreSecurity() {
  if (!security_changed) return;
  ble_hs_cfg.sm_io_cap = saved_io;
  ble_hs_cfg.sm_bonding = saved_bonding;
  ble_hs_cfg.sm_mitm = saved_mitm;
  security_changed = false;
}
void Finish(const char *message) {
  generation++;
  state = message ? "error" : "done";
  error = message ? message : "";
  passkey.store(0);
  new_pin = 0;
  if (conn != BLE_HS_CONN_HANDLE_NONE) {
    ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
    conn = BLE_HS_CONN_HANDLE_NONE;
  }
  ble_gap_conn_cancel();
  if (mgos_bt_gap_scan_in_progress()) mgos_bt_gap_scan_stop();
  RestoreSecurity();
}
void FailStart(int result, const char *message) {
  if (result != 0) Finish(message);
}
void QueueError(const char *message, unsigned current) {
  mgos::InvokeCB([message, current] {
    if (current == generation && Active()) Finish(message);
  });
}
ble_uuid128_t UUID(const char *text) {
  mgos_bt_uuid uuid;
  ble_uuid_any_t native;
  mgos_bt_uuid_from_str(mg_mk_str(text), &uuid);
  mgos_bt_uuid_to_esp32(&uuid, &native);
  return native.u128;
}
int GapEvent(ble_gap_event *event, void *);
void Connect() {
  repeat_used.store(false);
  state = existing ? "connecting_secure" : "connecting";
  mgos_bt_gap_scan_stop();
  uint8_t own_type;
  if (ble_hs_id_infer_auto(0, &own_type) != 0) {
    Finish("Cannot select BLE address");
    return;
  }
  FailStart(
      ble_gap_connect(own_type, &native_peer, 12000, nullptr, GapEvent,
                      reinterpret_cast<void *>(uintptr_t(generation.load()))),
      "BLE connection failed");
}
void Discover();
void ReadKey();
int WriteResult(uint16_t, const ble_gatt_error *err, ble_gatt_attr *,
                void *arg) {
  const unsigned current = uintptr_t(arg);
  const int result = err->status;
  mgos::InvokeCB([current, result] {
    if (current != generation || !Active()) return;
    if (state != "enabling_beacon" && state != "setting_pin") return;
    if (result != 0) {
      Finish("Sensor rejected configuration; wake it and retry");
      return;
    }
    if (state == "enabling_beacon") {
      if (rotated) {
        ReadKey();
        return;
      }
      state = "setting_pin";
      uint32_t value = new_pin;
      uint8_t bytes[4] = {uint8_t(value), uint8_t(value >> 8),
                          uint8_t(value >> 16), uint8_t(value >> 24)};
      FailStart(ble_gattc_write_flat(
                    conn, pin_handle, bytes, 4, WriteResult,
                    reinterpret_cast<void *>(uintptr_t(generation.load()))),
                "Cannot set sensor PIN");
    } else if (state == "setting_pin") {
      rotated = true;
      passkey.store(new_pin);
      state = "reconnecting";
      reconnecting = true;
      FailStart(ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM),
                "Cannot reconnect sensor");
    }
  });
  return 0;
}
int KeyResult(uint16_t, const ble_gatt_error *err, ble_gatt_attr *attr,
              void *arg) {
  const unsigned current = uintptr_t(arg);
  std::array<uint8_t, 16> key = {};
  bool ok = err->status == 0 && attr != nullptr &&
            OS_MBUF_PKTLEN(attr->om) == 16 &&
            os_mbuf_copydata(attr->om, 0, 16, key.data()) == 0;
  mgos::InvokeCB([current, ok, key] {
    if (current != generation || !Active()) return;
    if (!ok) {
      Finish("Cannot read encryption key over the secured connection");
      return;
    }
    SecureRecord record = {};
    record.version = 1;
    memcpy(record.mac, peer.addr, 6);
    memcpy(record.key, key.data(), 16);
    SecureRecord old;
    // Re-pairing the same key must never reset replay protection.
    if (LoadRecord(role, &old) && memcmp(old.mac, record.mac, 6) == 0 &&
        memcmp(old.key, record.key, 16) == 0)
      record = old;
    if (!SaveRecord(role, record)) {
      Finish("Cannot save sensor key");
      return;
    }
    char mac[MGOS_BT_ADDR_STR_LEN];
    mgos_bt_addr_to_str(&peer, 0, mac);
    if (role == 0)
      mgos_sys_config_set_blu_closed_mac(mac);
    else
      mgos_sys_config_set_blu_open_mac(mac);
    if (!mgos_sys_config_save(&mgos_sys_config, false, nullptr)) {
      Finish("Cannot save sensor assignment; pair again");
      return;
    }
    ResetReadings();
    state = "waiting_encrypted_beacon";
    deadline = mgos_uptime() + 180;
    passkey.store(0);
    if (conn != BLE_HS_CONN_HANDLE_NONE)
      ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
    RestoreSecurity();
  });
  return 0;
}
void ReadKey() {
  state = "reading_key";
  FailStart(
      ble_gattc_read(conn, key_handle, KeyResult,
                     reinterpret_cast<void *>(uintptr_t(generation.load()))),
      "Cannot request encryption key");
}
int Characteristic(uint16_t, const ble_gatt_error *err, const ble_gatt_chr *chr,
                   void *arg) {
  const unsigned current = uintptr_t(arg);
  const int status = err->status;
  uint16_t handle = chr ? chr->val_handle : 0;
  int which = 0;
  if (chr) {
    auto pin_uuid = UUID("0ffb7104-860c-49ae-8989-1f946d5f6c03");
    auto key_uuid = UUID("eb0fb41b-af4b-4724-a6f9-974f55aba81a");
    auto beacon_uuid = UUID("cb9e957e-952d-4761-a7e1-4416494a5bfa");
    if (ble_uuid_cmp(&chr->uuid.u, &pin_uuid.u) == 0) which = 1;
    if (ble_uuid_cmp(&chr->uuid.u, &key_uuid.u) == 0) which = 2;
    if (ble_uuid_cmp(&chr->uuid.u, &beacon_uuid.u) == 0) which = 3;
  }
  mgos::InvokeCB([current, status, handle, which] {
    if (current != generation || !Active()) return;
    if (status == 0) {
      if (which == 1) pin_handle = handle;
      if (which == 2) key_handle = handle;
      if (which == 3) beacon_handle = handle;
    } else if (status == BLE_HS_EDONE) {
      if (!pin_handle || !key_handle || !beacon_handle) {
        Finish("Not a compatible BLU Door/Window sensor");
        return;
      }
      state = "enabling_beacon";
      uint8_t enabled = 1;
      FailStart(ble_gattc_write_flat(
                    conn, beacon_handle, &enabled, 1, WriteResult,
                    reinterpret_cast<void *>(uintptr_t(generation.load()))),
                "Cannot enable sensor beacons");
    } else {
      Finish("Characteristic discovery failed");
    }
  });
  return 0;
}
int Service(uint16_t, const ble_gatt_error *err, const ble_gatt_svc *svc,
            void *arg) {
  const unsigned current = uintptr_t(arg);
  const int status = err->status;
  const uint16_t start = svc ? svc->start_handle : 0;
  const uint16_t end = svc ? svc->end_handle : 0;
  mgos::InvokeCB([current, status, start, end] {
    if (current != generation || !Active()) return;
    if (status == 0) {
      svc_start = start;
      svc_end = end;
    } else if (status == BLE_HS_EDONE && svc_start != 0) {
      FailStart(ble_gattc_disc_all_chrs(
                    conn, svc_start, svc_end, Characteristic,
                    reinterpret_cast<void *>(uintptr_t(generation.load()))),
                "Cannot discover sensor characteristics");
    } else {
      Finish("Shelly service not found");
    }
  });
  return 0;
}
void Discover() {
  state = "discovering_services";
  svc_start = svc_end = pin_handle = key_handle = beacon_handle = 0;
  auto uuid = UUID("de8a5aac-a99b-c315-0c80-60d4cbb51225");
  FailStart(ble_gattc_disc_svc_by_uuid(
                conn, &uuid.u, Service,
                reinterpret_cast<void *>(uintptr_t(generation.load()))),
            "Cannot discover Shelly service");
}
int GapEvent(ble_gap_event *event, void *arg) {
  const unsigned current = uintptr_t(arg);
  if (event->type == BLE_GAP_EVENT_PASSKEY_ACTION) {
    if (current != generation ||
        event->passkey.params.action != BLE_SM_IOACT_INPUT ||
        passkey.load() == 0)
      return BLE_HS_ENOTSUP;
    ble_sm_io io = {};
    io.action = BLE_SM_IOACT_INPUT;
    io.passkey = passkey.load();
    return ble_sm_inject_io(event->passkey.conn_handle, &io);
  }
  if (event->type == BLE_GAP_EVENT_REPEAT_PAIRING) {
    // Only this user-selected outgoing connection may replace a stale bond,
    // once per attempt. PIN authentication is still required for secure peers.
    if (current == generation && passkey.load() != 0 &&
        !repeat_used.exchange(true)) {
      ble_gap_conn_desc desc = {};
      if (ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc) == 0 &&
          ble_store_util_delete_peer(&desc.peer_id_addr) == 0)
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }
    QueueError("BLE bond could not be renewed; wake the sensor and retry",
               current);
    return BLE_GAP_REPEAT_PAIRING_IGNORE;
  }
  if (event->type == BLE_GAP_EVENT_CONNECT) {
    const int status = event->connect.status;
    const uint16_t handle = event->connect.conn_handle;
    mgos::InvokeCB([current, status, handle] {
      if (current != generation || !Active()) {
        if (status == 0) ble_gap_terminate(handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
      }
      if (status != 0) {
        if (rotated) {
          state = "waiting_pairing_mode";
          deadline = mgos_uptime() + 60;
        } else
          Finish("Connection failed; put the sensor in pairing mode and retry");
        return;
      }
      conn = handle;
      state = "securing_link";
      int result = ble_gap_security_initiate(conn);
      if (result == BLE_HS_EALREADY) {
        ble_gap_conn_desc desc = {};
        if (ble_gap_conn_find(conn, &desc) == 0 && desc.sec_state.encrypted &&
            desc.sec_state.bonded &&
            (!existing || desc.sec_state.authenticated))
          Discover();
        else
          Finish("BLE link is not authenticated");
      } else
        FailStart(result, "Cannot initiate BLE security");
    });
  } else if (event->type == BLE_GAP_EVENT_ENC_CHANGE) {
    ble_gap_conn_desc desc = {};
    bool ok = event->enc_change.status == 0 &&
              ble_gap_conn_find(event->enc_change.conn_handle, &desc) == 0 &&
              desc.sec_state.encrypted && desc.sec_state.bonded;
    bool authenticated = desc.sec_state.authenticated;
    mgos::InvokeCB([current, ok, authenticated] {
      if (current != generation || !Active()) return;
      if (state != "securing_link") return;
      if (!ok || (existing && !authenticated)) {
        Finish("BLE authentication failed; check the PIN");
        return;
      }
      Discover();
    });
  } else if (event->type == BLE_GAP_EVENT_DISCONNECT) {
    const int reason = event->disconnect.reason;
    mgos::InvokeCB([current, reason] {
      if (current != generation) return;
      conn = BLE_HS_CONN_HANDLE_NONE;
      if (state == "waiting_encrypted_beacon" || !Active()) return;
      if (reconnecting || state == "setting_pin") {
        // Changing the PIN rotates the sensor's key; establish a new
        // authenticated bond.
        reconnecting = false;
        rotated = true;
        passkey.store(new_pin);
        existing = true;
        ble_store_util_delete_peer(&native_peer);
        ble_hs_cfg.sm_mitm = 1;
        Connect();
      } else {
        const std::string message = mgos::SPrintf(
            "Sensor disconnected during %s (BLE reason 0x%02x); wake it and retry",
            state.c_str(), reason);
        LOG(LL_WARN, ("BLU pairing: %s", message.c_str()));
        Finish(message.c_str());
      }
    });
  }
  return 0;
}

bool Authorized(mg_rpc_request_info *ri) {
  if (mgos_conf_str_empty(mgos_sys_config_get_rpc_auth_file())) {
    mg_rpc_send_errorf(ri, 403,
                       "Set a web panel password before pairing sensors");
    return false;
  }
  return true;
}
void StatusRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *, mg_str) {
  std::string list = "[";
  for (const auto &candidate : candidates) {
    if (mgos_uptime() - candidate.seen > 15) continue;
    char mac[MGOS_BT_ADDR_STR_LEN];
    mgos_bt_addr_to_str(&candidate.addr, 0, mac);
    if (list.size() > 1) list += ",";
    list += mgos::JSONPrintStringf("{mac:%Q,rssi:%d,encrypted:%B}", mac,
                                   candidate.rssi, candidate.encrypted);
  }
  list += "]";
  mg_rpc_send_responsef(
      ri, "{state:%Q,error:%Q,role:%d,remaining_s:%d,candidates:%s}",
      state.c_str(), error.c_str(), role,
      Active() ? int(deadline - mgos_uptime()) : 0, list.c_str());
}
void DiscoverRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *, mg_str) {
  if (!Authorized(ri)) return;
  if (mgos_bt_get_status() != MGOS_BT_STARTED) {
    mg_rpc_send_errorf(
        ri, 503, "Bluetooth is not running; enable it and restart Shelly");
    return;
  }
  if (Active()) {
    mg_rpc_send_errorf(ri, 409, "Pairing already active");
    return;
  }
  state = "scanning";
  error.clear();
  role = -1;
  candidates.clear();
  deadline = mgos_uptime() + 60;
  mgos_bt_gap_scan_stop();
  mg_rpc_send_responsef(ri, "{}");
}
void PairRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *,
             mg_str args) {
  if (!Authorized(ri)) return;
  char *mac = nullptr, *pin = nullptr, *current_pin = nullptr;
  int selected_role = -1;
  json_scanf(args.p, args.len, "{role:%d,mac:%Q,pin:%Q,current_pin:%Q}",
             &selected_role, &mac, &pin, &current_pin);
  mgos::ScopedCPtr mac_owner(mac), pin_owner(pin), current_owner(current_pin);
  if (state != "scanning" || selected_role < 0 || selected_role > 1 ||
      !ValidMAC(mac) || pin == nullptr || strlen(pin) != 6 || pin[0] == '0') {
    mg_rpc_send_errorf(
        ri, 400,
        "Select a discovered sensor, role and a six-digit PIN (100000-999999)");
    return;
  }
  uint32_t value = 0;
  for (int i = 0; i < 6; i++) {
    if (pin[i] < '0' || pin[i] > '9') {
      mg_rpc_send_errorf(ri, 400, "Invalid PIN");
      return;
    }
    value = value * 10 + pin[i] - '0';
  }
  mgos_bt_addr selected;
  mgos_bt_addr_from_str(mg_mk_str(mac), &selected);
  const Candidate *candidate = nullptr;
  for (const auto &item : candidates) {
    if (memcmp(item.addr.addr, selected.addr, 6) == 0 &&
        mgos_uptime() - item.seen <= 15)
      candidate = &item;
  }
  if (!candidate) {
    mg_rpc_send_errorf(ri, 400, "Sensor expired; scan and wake it again");
    return;
  }
  const char *other = selected_role == 0 ? mgos_sys_config_get_blu_open_mac()
                                         : mgos_sys_config_get_blu_closed_mac();
  if (other && strcasecmp(mac, other) == 0) {
    mg_rpc_send_errorf(ri, 400,
                       "Sensor is already assigned to the other endpoint");
    return;
  }
  uint32_t old_value = 0;
  if (candidate->encrypted) {
    if (current_pin == nullptr || strlen(current_pin) != 6 ||
        current_pin[0] == '0') {
      mg_rpc_send_errorf(ri, 400,
                         "Enter the current PIN of this secured sensor, or "
                         "factory-reset it first");
      return;
    }
    for (int i = 0; i < 6; i++) {
      if (current_pin[i] < '0' || current_pin[i] > '9') {
        mg_rpc_send_errorf(ri, 400, "Invalid current PIN");
        return;
      }
      old_value = old_value * 10 + current_pin[i] - '0';
    }
    if (old_value == value) {
      mg_rpc_send_errorf(
          ri, 400, "Choose a different new PIN to rotate the encryption key");
      return;
    }
  }
  role = selected_role;
  peer = candidate->addr;
  existing = candidate->encrypted;
  mgos_bt_addr_to_esp32(&peer, &native_peer);
  saved_io = ble_hs_cfg.sm_io_cap;
  saved_bonding = ble_hs_cfg.sm_bonding;
  saved_mitm = ble_hs_cfg.sm_mitm;
  security_changed = true;
  ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_KEYBOARD_ONLY;
  ble_hs_cfg.sm_bonding = 1;
  ble_hs_cfg.sm_mitm = existing ? 1 : 0;
  new_pin = value;
  rotated = false;
  passkey.store(existing ? old_value : value);
  reconnecting = false;
  deadline = mgos_uptime() + 90;
  generation++;
  Connect();
  mg_rpc_send_responsef(ri, "{}");
}
void CancelRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *, mg_str) {
  if (!Authorized(ri)) return;
  Finish("Pairing cancelled");
  mg_rpc_send_responsef(ri, "{}");
}
}  // namespace

bool PairingScan() {
  return state == "scanning" || state == "waiting_encrypted_beacon" ||
         state == "waiting_pairing_mode";
}
bool PairingConnecting() {
  return Active() && !PairingScan();
}
bool PairingBusy() {
  return Active();
}
void PairingVerified(int verified_role) {
  if (state == "waiting_encrypted_beacon" && verified_role == role)
    Finish(nullptr);
}
void PairingAdvertisement(void *data) {
  if (state != "scanning" && state != "waiting_pairing_mode") return;
  auto *result = static_cast<mgos_bt_gap_scan_result *>(data);
  mg_str manufacturer = mgos_bt_gap_parse_adv_data(
      result->scan_rsp, MGOS_BT_GAP_EIR_MANUFACTURER_SPECIFIC_DATA);
  if (manufacturer.len < 2 || uint8_t(manufacturer.p[0]) != 0xa9 ||
      uint8_t(manufacturer.p[1]) != 0x0b)
    return;
  bool pairing = false, encrypted = false, door = false;
  for (size_t i = 2; i < manufacturer.len;) {
    uint8_t type = manufacturer.p[i++];
    size_t n = type == 1 || type == 0x0b ? 2 : type == 0x0a ? 6 : 0;
    if (!n || n > manufacturer.len - i) return;
    if (type == 1) {
      pairing = manufacturer.p[i] & 0x10;
      encrypted = manufacturer.p[i] & 2;
    }
    if (type == 0x0b)
      door = uint8_t(manufacturer.p[i]) == 2 && manufacturer.p[i + 1] == 0;
    i += n;
  }
  if (!pairing || !door) return;
  if (state == "waiting_pairing_mode") {
    if (memcmp(peer.addr, result->addr.addr, 6) == 0) Connect();
    return;
  }
  for (auto &candidate : candidates) {
    if (memcmp(candidate.addr.addr, result->addr.addr, 6) == 0) {
      candidate.rssi = result->rssi;
      candidate.encrypted = encrypted;
      candidate.seen = mgos_uptime();
      return;
    }
  }
  if (candidates.size() < 8)
    candidates.push_back(
        {result->addr, result->rssi, encrypted, mgos_uptime()});
}
void PairingInit() {
  auto *rpc = mgos_rpc_get_global();
  mg_rpc_add_handler(rpc, "Shelly.BLU.Discover", "", DiscoverRPC, nullptr);
  mg_rpc_add_handler(rpc, "Shelly.BLU.Pair",
                     "{role:%d,mac:%Q,pin:%Q,current_pin:%Q}", PairRPC,
                     nullptr);
  mg_rpc_add_handler(rpc, "Shelly.BLU.Status", "", StatusRPC, nullptr);
  mg_rpc_add_handler(rpc, "Shelly.BLU.Cancel", "", CancelRPC, nullptr);
  mgos_set_timer(
      500, MGOS_TIMER_REPEAT,
      [](void *) {
        if (Active() && mgos_uptime() >= deadline)
          Finish("Pairing timed out; wake the sensor and retry");
      },
      nullptr);
}
}  // namespace blu
}  // namespace shelly
#endif
