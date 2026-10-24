#include "shelly_blu_pairing.hpp"
#ifdef SHELLY_GARAGE_BLE
#include <string.h>
#include <array>
#include <atomic>
#include <string>
#include <vector>
#include "esp32xx_bt.h"
#include "esp_system.h"
#include "mgos.hpp"
#include "mgos_bt_gap.h"
#include "mgos_rpc.h"
#include "mgos_sys_config.h"
extern "C" {
#include "host/ble_gap.h"
#include "host/ble_att.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "host/ble_store.h"
#include "host/util/util.h"
}
#include "shelly_blu_garage.hpp"
#include "shelly_blu_keys.hpp"
#include "shelly_blu_xiaomi.hpp"
#include "shelly_blu_xiaomi_gatt.hpp"
#include "shelly_main.hpp"

namespace shelly {
namespace blu {
namespace {
struct Candidate {
  mgos_bt_addr addr;
  int rssi;
  bool encrypted;  // Legacy RPC field name: BLE authentication enabled.
  uint16_t model;
  double seen;
};
std::vector<Candidate> candidates;
std::string state = "idle", error;
double deadline = 0;
int role = -1;
int discovery_model = 0;
mgos_bt_addr peer = {};
ble_addr_t native_peer = {};
std::atomic<uint32_t> passkey(0);
uint32_t new_pin = 0;
bool rotated = false;
bool key_writable = false, key_provision_used = false;
std::array<uint8_t, 16> pending_key = {};
uint16_t conn = BLE_HS_CONN_HANDLE_NONE;
uint16_t svc_start = 0, svc_end = 0, pin_handle = 0, key_handle = 0,
         beacon_handle = 0;
bool existing = false;
bool security_pair_required = false;
bool security_bond_resume = false;
bool security_pairing_started = false, security_retry_used = false;
enum class BondRenewal { None, WaitingForPairingMode, Ready, Used };
BondRenewal bond_renewal = BondRenewal::None;
unsigned connection_attempts = 0, key_read_attempts = 0, empty_key_reads = 0;
bool key_reconnect_used = false;
unsigned security_ticket = 0;
uint8_t saved_io = 0, saved_bonding = 0, saved_mitm = 0;
uint8_t saved_our_keys = 0, saved_their_keys = 0;
std::string diagnostics;
double attempt_started = 0;
void Trace(const std::string &entry) {
  if (diagnostics.size() > 1024) return;
  if (!diagnostics.empty()) diagnostics += "; ";
  diagnostics += mgos::SPrintf("%.2fs %s", mgos_uptime() - attempt_started,
                              entry.c_str());
}
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
  ble_hs_cfg.sm_our_key_dist = saved_our_keys;
  ble_hs_cfg.sm_their_key_dist = saved_their_keys;
  security_changed = false;
}
void Finish(const char *message) {
  const bool xiaomi_read = state == "reading_xiaomi";
  if (xiaomi_read) XiaomiGATTCancel();
  generation++;
  security_ticket++;
  state = message ? "error" : "done";
  error = message ? message : "";
  if (message && !diagnostics.empty()) error += " [" + diagnostics + "]";
  passkey.store(0);
  new_pin = 0;
  pending_key.fill(0);
  if (xiaomi_read) return;  // This client owns its connection; keep the scan running.
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
bool RenewSelectedBond(uint16_t handle) {
  // Authentication-disabled advertising does not mean no bond exists.
  // Replace only the active selected peer, on a repeat-pairing request or
  // after its stored bond timed out and the user reopened its pairing window.
  if (!Active() || handle != conn || (existing && passkey.load() == 0) ||
      repeat_used.exchange(true)) return false;
  ble_gap_conn_desc desc = {};
  if (ble_gap_conn_find(handle, &desc) != 0) return false;
  const int result = ble_store_util_delete_peer(&desc.peer_id_addr);
  Trace(mgos::SPrintf("renew_selected_bond=0x%x", result));
  return result == 0 || result == BLE_HS_ENOENT;
}
void Connect() {
  repeat_used.store(false);
  connection_attempts++;
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
bool ReusablePeerBond() {
  security_pair_required = false;
  ble_gap_conn_desc desc = {};
  const int lookup = ble_gap_conn_find(conn, &desc);
  if (lookup != 0) return false;
  ble_store_key_sec key = {};
  key.peer_addr = desc.peer_id_addr;
  ble_store_value_sec value = {};
  const int result = ble_store_read_peer_sec(&key, &value);
  const bool ltk = result == 0 && value.ltk_present;
  const bool authenticated = result == 0 && value.authenticated;
  // Report metadata only. Never include LTKs, IRKs or PINs in diagnostics.
  Trace(mgos::SPrintf("bond=0x%x ltk=%d auth=%d", result, ltk, authenticated));
  security_pair_required = ltk && existing && !authenticated;
  return ltk && !security_pair_required;
}
void StartSecurity() {
  if (!Active() || state != "securing_link" || conn == BLE_HS_CONN_HANDLE_NONE)
    return;
  ble_gap_conn_desc desc = {};
  const int lookup = ble_gap_conn_find(conn, &desc);
  if (lookup != 0) {
    const std::string message = mgos::SPrintf("BLE connection unavailable before security (0x%x)", lookup);
    Finish(message.c_str());
    return;
  }
  // The peripheral may already have initiated security while we waited.
  if (desc.sec_state.encrypted) {
    if (desc.sec_state.bonded && (!existing || desc.sec_state.authenticated)) {
      Discover();
      return;
    }
    if (!security_pair_required) {
      Finish("BLE link does not satisfy the required bonding/authentication");
      return;
    }
  }
  // Explicit pairing upgrades legacy authentication or renews a transport
  // bond whose encryption restore timed out in the previous connection.
  const int result = security_pair_required ? ble_gap_pair_initiate(conn)
                                            : ble_gap_security_initiate(conn);
  if (!security_bond_resume && (result == 0 || result == BLE_HS_EALREADY))
    security_pairing_started = true;
  Trace(mgos::SPrintf("%s=0x%x",
                      security_pair_required ? "pairing_start" : "security_start", result));
  // EALREADY is an in-progress procedure, not an authentication failure.
  // Its ENC_CHANGE event must still validate encryption, bonding and MITM.
  if (result != BLE_HS_EALREADY)
    FailStart(result, "Cannot initiate BLE security");
}
void ScheduleSecurity() {
  state = "securing_link";
  const unsigned ticket = ++security_ticket;
  deadline = mgos_uptime() + 15;
  const bool reusable_bond = ReusablePeerBond();
  security_bond_resume = false;
  security_pairing_started = false;
  if (bond_renewal == BondRenewal::Ready) {
    // Ready is armed only by the selected sensor's fresh pairing advertisement.
    // Remove that peer's failed transport bond before
    // initiating SMP, also freeing its slot instead of evicting another peer.
    bond_renewal = BondRenewal::Used;
    if (!RenewSelectedBond(conn)) {
      Finish("Cannot renew the selected device's BLE bond");
      return;
    }
    security_pair_required = true;
  }
  security_bond_resume = reusable_bond && !security_pair_required;
  // Resume a stored bond immediately. Otherwise a peripheral Security Request
  // without the bonding flag can make NimBLE start new pairing first.
  if (reusable_bond || security_pair_required) { StartSecurity(); return; }
  if (role != 0 && role != 1 && role != 2 && role != 5) { StartSecurity(); return; }
  Trace("security_wait=500ms");
  mgos_set_timer(500, 0, [](void *arg) {
    if (uintptr_t(arg) == security_ticket) StartSecurity();
  }, reinterpret_cast<void *>(uintptr_t(ticket)));
}
void ReadKey();
void QueueKeyRead();
void SetPIN();
void StoreKey(const std::array<uint8_t, 16> &key);
void ConfigureEncryption();
bool ResumeSecurityAfterDisconnect(int reason) {
  // BLU can close the connection while replacing its own stale bond. Give
  // fresh SMP one reconnect, never retry an authentication failure or a
  // failed LTK restore. A new pairing advertisement is required in all cases.
  if (state != "securing_link" || !security_pairing_started ||
      security_bond_resume || security_retry_used ||
      reason != BLE_HS_ERR_HCI_BASE + BLE_ERR_REM_USER_CONN_TERM) return false;
  security_retry_used = true;
  generation++;
  security_ticket++;
  state = "waiting_security_retry";
  deadline = mgos_uptime() + 5;
  Trace("Peer closed fresh pairing; waiting for selected device to retry once");
  return true;
}
void WaitForSecurityRetryWake() {
  state = "waiting_user_pairing";
  deadline = mgos_uptime() + 180;
  Trace("Retry needs pairing mode; hold button again; automatic resume");
}
bool ResumeKeyAfterDisconnect() {
  if (!rotated || key_reconnect_used ||
      (state != "waiting_key_ready" && state != "reading_key" &&
       state != "reconnecting_secure")) return false;
  key_reconnect_used = true;
  key_read_attempts = 0;
  empty_key_reads = 0;
  // Invalidate callbacks/timers from the lost connection. Keep the negotiated
  // bond and security policy; a transport timeout is not a PIN failure.
  generation++;
  security_ticket++;
  if (role != 2 || state == "reconnecting_secure") {
    // Connecting consumes the physical pairing window. After PIN rotation,
    // wait for a fresh advertisement to learn the authentication policy.
    // BLU 1.0.22+ keeps PIN configuration but no longer uses PIN bonding.
    state = "waiting_user_pairing";
    deadline = mgos_uptime() + 180;
    Trace("Key read interrupted; enable pairing mode again; preserving bond");
    return true;
  }
  Trace("resuming key read after disconnect; preserving bond");
  Connect();
  return true;
}
void ConfigureReconnectAuthentication(bool auth_enabled) {
  // This flag is BLE authentication, not BTHome payload encryption. Modern
  // firmware can advertise auth=0 after successfully configuring encryption.
  existing = auth_enabled;
  passkey.store(existing ? new_pin : 0);
  ble_hs_cfg.sm_io_cap = existing ? BLE_SM_IO_CAP_KEYBOARD_ONLY
                                  : BLE_SM_IO_CAP_NO_IO;
  ble_hs_cfg.sm_mitm = existing ? 1 : 0;
  Trace(mgos::SPrintf("reconnect_auth=%d", existing));
}
int WriteResult(uint16_t, const ble_gatt_error *err, ble_gatt_attr *,
                void *arg) {
  const unsigned current = uintptr_t(arg);
  const int result = err->status;
  mgos::InvokeCB([current, result] {
    if (current != generation || !Active()) return;
    if (state != "enabling_beacon" && state != "setting_pin") return;
    if (result != 0) {
      const std::string message = mgos::SPrintf(
          "Device rejected %s (GATT 0x%x)",
          state == "enabling_beacon" ? "beacon configuration" : "PIN configuration",
          result);
      Trace(mgos::SPrintf("configuration_write=0x%x stage=%s", result, state.c_str()));
      Finish(message.c_str());
      return;
    }
    if (state == "enabling_beacon") {
      if (rotated || !new_pin) {
        ReadKey();
        return;
      }
      SetPIN();
    } else if (state == "setting_pin") {
      rotated = true;
      // Key generation may complete after the PIN write response. Keep this
      // bonded link, wait briefly, then read without rotating the PIN again.
      QueueKeyRead();
    }
  });
  return 0;
}
void SetPIN() {
  if (!pin_handle) { Finish("Device has no PIN-based encryption setup"); return; }
  state = "setting_pin";
  const uint32_t value = new_pin;
  const uint8_t bytes[4] = {uint8_t(value), uint8_t(value >> 8),
                           uint8_t(value >> 16), uint8_t(value >> 24)};
  FailStart(ble_gattc_write_flat(conn, pin_handle, bytes, 4, WriteResult,
            reinterpret_cast<void *>(uintptr_t(generation.load()))), "Cannot set device PIN");
}
int KeyWriteResult(uint16_t, const ble_gatt_error *err, ble_gatt_attr *, void *arg) {
  const unsigned current = uintptr_t(arg);
  const int result = err->status;
  mgos::InvokeCB([current, result] {
    if (current != generation || !Active() || state != "setting_key") return;
    Trace(mgos::SPrintf("key_write=0x%x", result));
    if (result != 0) {
      Finish("Device rejected encryption setup; no new key was saved");
      return;
    }
    // The acknowledged key is already known. No PIN rotation or second
    // connection is needed to fetch it; encrypted beacons must still verify it.
    StoreKey(pending_key);
    pending_key.fill(0);
  });
  return 0;
}
void ConfigureEncryption() {
  if (key_provision_used) { Finish("Device did not provide an encryption key"); return; }
  key_provision_used = true;
  if (key_writable && !existing) {
    // Modern BLU supports setting a BTHome key directly. Require the actual
    // GATT write-with-response property, never infer it from auth advertising.
    esp_fill_random(pending_key.data(), pending_key.size());
    state = "setting_key";
    FailStart(ble_gattc_write_flat(conn, key_handle, pending_key.data(), pending_key.size(),
                  KeyWriteResult, reinterpret_cast<void *>(uintptr_t(generation.load()))),
              "Cannot configure device encryption");
  } else {
    if (!pin_handle) { Finish("Device has no supported encryption setup"); return; }
    state = "waiting_encryption_pin";
    deadline = mgos_uptime() + 180;
    Trace("Device needs encryption activation via PIN");
  }
}
int KeyResult(uint16_t, const ble_gatt_error *err, ble_gatt_attr *attr,
              void *arg) {
  const unsigned current = uintptr_t(arg);
  std::array<uint8_t, 16> key = {};
  const int status = err->status;
  const int length = attr && attr->om ? OS_MBUF_PKTLEN(attr->om) : -1;
  bool ok = status == 0 && length == 16 &&
            os_mbuf_copydata(attr->om, 0, 16, key.data()) == 0;
  uint8_t first = 0xff;
  const bool empty = status == 0 && (length == 0 ||
      (length == 1 && os_mbuf_copydata(attr->om, 0, 1, &first) == 0 && first == 0));
  mgos::InvokeCB([current, ok, empty, key, status, length] {
    if (current != generation || !Active() || state != "reading_key") return;
    Trace(mgos::SPrintf("key_read=0x%x len=%d", status, length));
    empty_key_reads = empty ? empty_key_reads + 1 : 0;
    if (!rotated && !key_provision_used && empty_key_reads >= 3) {
      ConfigureEncryption();
      return;
    }
    if (!ok) {
      // NimBLE completes pending GATT reads with ENOTCONN before delivering
      // DISCONNECT. Let that event resume once instead of scheduling a dead read.
      if (status == BLE_HS_ENOTCONN && rotated && !key_reconnect_used) return;
      const bool authentication_required =
          status == BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN) ||
          status == BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC);
      if (authentication_required && rotated && new_pin && !key_reconnect_used) {
        key_read_attempts = 0;
        state = "reconnecting_secure";
        Trace("key requires a fresh secured connection");
        FailStart(ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM),
                  "Cannot reconnect to authenticate the device key");
      } else if (!authentication_required && key_read_attempts < 3) {
        QueueKeyRead();
      } else {
        const std::string message = mgos::SPrintf(
            "Cannot read encryption key (GATT 0x%x, length %d); device PIN may already be set",
            status, length);
        Finish(message.c_str());
      }
      return;
    }
    StoreKey(key);
  });
  return 0;
}
void StoreKey(const std::array<uint8_t, 16> &key) {
  SecureRecord record = {};
  record.version = 1;
  memcpy(record.mac, peer.addr, 6);
  memcpy(record.key, key.data(), 16);
  SecureRecord old;
  // Re-pairing the same key must never reset replay protection.
  if (LoadRecord(role, &old) && old.protocol == kProtocolBTHome &&
      memcmp(old.mac, record.mac, 6) == 0 &&
      memcmp(old.key, record.key, 16) == 0)
    record = old;
  if (!SaveRecord(role, record)) {
    Finish("Cannot save device key");
    return;
  }
  char mac[MGOS_BT_ADDR_STR_LEN];
  mgos_bt_addr_to_str(&peer, 0, mac);
  if (role == 0)
    mgos_sys_config_set_blu_closed_mac(mac);
  else if (role == 1)
    mgos_sys_config_set_blu_open_mac(mac);
  else if (role == 2) mgos_sys_config_set_blu_motion_mac(mac);
  else if (role == 3) mgos_sys_config_set_blu_indoor_mac(mac);
  else if (role == 4) mgos_sys_config_set_blu_outdoor_mac(mac);
  else mgos_sys_config_set_blu_button_mac(mac);
  if (!mgos_sys_config_save(&mgos_sys_config, false, nullptr)) {
    Finish("Cannot save device assignment; pair again");
    return;
  }
  ResetReadings();
  state = "waiting_encrypted_beacon";
  deadline = mgos_uptime() + 180;
  passkey.store(0);
  if (conn != BLE_HS_CONN_HANDLE_NONE)
    ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
  RestoreSecurity();
}
void QueueKeyRead() {
  state = "waiting_key_ready";
  mgos_set_timer(500, 0, [](void *arg) {
    if (uintptr_t(arg) == generation && Active() &&
        state == "waiting_key_ready" && conn != BLE_HS_CONN_HANDLE_NONE)
      ReadKey();
  }, reinterpret_cast<void *>(uintptr_t(generation.load())));
}
void ReadKey() {
  key_read_attempts++;
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
  const bool writable = chr && (chr->properties & BLE_GATT_CHR_PROP_WRITE);
  int which = 0;
  if (chr) {
    auto pin_uuid = UUID("0ffb7104-860c-49ae-8989-1f946d5f6c03");
    auto key_uuid = UUID("eb0fb41b-af4b-4724-a6f9-974f55aba81a");
    auto beacon_uuid = UUID("cb9e957e-952d-4761-a7e1-4416494a5bfa");
    if (ble_uuid_cmp(&chr->uuid.u, &pin_uuid.u) == 0) which = 1;
    if (ble_uuid_cmp(&chr->uuid.u, &key_uuid.u) == 0) which = 2;
    if (ble_uuid_cmp(&chr->uuid.u, &beacon_uuid.u) == 0) which = 3;
  }
  mgos::InvokeCB([current, status, handle, which, writable] {
    if (current != generation || !Active()) return;
    if (status == 0) {
      if (which == 1) pin_handle = handle;
      if (which == 2) { key_handle = handle; key_writable = writable; }
      if (which == 3) beacon_handle = handle;
    } else if (status == BLE_HS_EDONE) {
      if (!key_handle || ((role < 3 || role == 5) && !beacon_handle)) {
        Finish("Device lacks required secure BLU characteristics");
        return;
      }
      // H&T broadcasts every minute by design and has no beacon switch.
      if (role == 3 || role == 4) {
        if (rotated || !new_pin) ReadKey();
        else SetPIN();
        return;
      }
      state = "enabling_beacon";
      uint8_t enabled = 1;
      FailStart(ble_gattc_write_flat(
                    conn, beacon_handle, &enabled, 1, WriteResult,
                    reinterpret_cast<void *>(uintptr_t(generation.load()))),
                "Cannot enable device beacons");
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
                "Cannot discover device characteristics");
    } else {
      Finish("Shelly service not found");
    }
  });
  return 0;
}
void Discover() {
  state = "discovering_services";
  deadline = mgos_uptime() + 90;
  svc_start = svc_end = pin_handle = key_handle = beacon_handle = 0;
  key_writable = false;
  auto uuid = UUID("de8a5aac-a99b-c315-0c80-60d4cbb51225");
  FailStart(ble_gattc_disc_svc_by_uuid(
                conn, &uuid.u, Service,
                reinterpret_cast<void *>(uintptr_t(generation.load()))),
            "Cannot discover Shelly service");
}
int GapEvent(ble_gap_event *event, void *arg) {
  const unsigned current = uintptr_t(arg);
  // Only event types and status codes: never record PINs, keys or SMP payloads.
  const int type = event->type;
  int detail = 0;
  if (type == BLE_GAP_EVENT_CONNECT) detail = event->connect.status;
  if (type == BLE_GAP_EVENT_ENC_CHANGE) detail = event->enc_change.status;
  if (type == BLE_GAP_EVENT_PASSKEY_ACTION) detail = event->passkey.params.action;
  if (type == BLE_GAP_EVENT_DISCONNECT) detail = event->disconnect.reason;
  mgos::InvokeCB([current, type, detail] {
    if (current == generation && Active())
      Trace(mgos::SPrintf("GAP %d/0x%x", type, detail));
  });
  if (event->type == BLE_GAP_EVENT_PASSKEY_ACTION) {
    LOG(LL_INFO, ("BLU pairing: passkey action %d", event->passkey.params.action));
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
    if (current == generation &&
        RenewSelectedBond(event->repeat_pairing.conn_handle))
      return BLE_GAP_REPEAT_PAIRING_RETRY;
    QueueError("BLE bond could not be renewed; wake the device and retry",
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
        if ((status == BLE_HS_ETIMEOUT || rotated) && connection_attempts < 3) {
          Trace("waiting for fresh pairing advertisement");
          state = "waiting_pairing_mode";
          deadline = mgos_uptime() + 60;
        } else
          Finish("Connection failed; put the device in pairing mode and retry");
        return;
      }
      conn = handle;
      ScheduleSecurity();
    });
  } else if (event->type == BLE_GAP_EVENT_ENC_CHANGE) {
    LOG(LL_INFO, ("BLU pairing: encryption status 0x%x", event->enc_change.status));
    ble_gap_conn_desc desc = {};
    const int lookup = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
    bool ok = event->enc_change.status == 0 && lookup == 0 &&
              desc.sec_state.encrypted && desc.sec_state.bonded;
    bool authenticated = desc.sec_state.authenticated;
    const int status = event->enc_change.status;
    const bool encrypted = desc.sec_state.encrypted, bonded = desc.sec_state.bonded;
    mgos::InvokeCB([current, ok, authenticated, status, encrypted, bonded, lookup] {
      if (current != generation || !Active()) return;
      if (state != "securing_link") return;
      if (!ok || (existing && !authenticated)) {
        const std::string message = mgos::SPrintf(
            "BLE security failed (status 0x%x, encrypted=%d, bonded=%d, authenticated=%d, lookup=0x%x)",
            status, encrypted, bonded, authenticated, lookup);
        Finish(message.c_str());
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
      if (ResumeSecurityAfterDisconnect(reason)) return;
      if (ResumeKeyAfterDisconnect()) return;
      const std::string message = mgos::SPrintf(
          "Device disconnected during %s (BLE reason 0x%02x); wake it and retry",
          state.c_str(), reason);
      LOG(LL_WARN, ("BLU pairing: %s", message.c_str()));
      Finish(message.c_str());
    });
  }
  return 0;
}

bool Authorized(mg_rpc_request_info *ri) {
  if (mgos_conf_str_empty(mgos_sys_config_get_rpc_auth_file())) {
    mg_rpc_send_errorf(ri, 403,
                       "Set a web panel password before pairing devices");
    return false;
  }
  return true;
}
void WaitForUserPairing() {
  if (bond_renewal == BondRenewal::Used || security_retry_used) {
    Finish("Device did not complete renewed BLE pairing; wake it and start a new attempt");
    return;
  }
  if (security_bond_resume && bond_renewal == BondRenewal::None) {
    bond_renewal = BondRenewal::WaitingForPairingMode;
    Trace("Stored bond timed out; renewal needs a fresh pairing window");
  }
  // Abort this stalled security attempt without deleting bonds or sensor keys.
  // Ignore queued events from its connection. Reconnect only on a fresh
  // pairing advertisement from the selected peer; other sensors are ignored.
  generation++;
  security_ticket++;
  const uint16_t old_conn = conn;
  conn = BLE_HS_CONN_HANDLE_NONE;
  state = "waiting_user_pairing";
  deadline = mgos_uptime() + 180;
  Trace("Security timed out; waiting for selected device pairing mode");
  if (old_conn != BLE_HS_CONN_HANDLE_NONE)
    ble_gap_terminate(old_conn, BLE_ERR_REM_USER_CONN_TERM);
}
bool ResumePairingWindow() {
  if (state != "waiting_user_pairing") return false;
  state = "waiting_pairing_mode";
  deadline = mgos_uptime() + 60;
  Trace("User confirmed pairing mode; waiting for selected device advertisement");
  return true;
}
void ResumeRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *, mg_str) {
  if (!Authorized(ri)) return;
  if (!ResumePairingWindow()) {
    mg_rpc_send_errorf(ri, 409, "No device pairing is waiting for confirmation");
    return;
  }
  mg_rpc_send_responsef(ri, "{}");
}
bool ParsePIN(const char *pin, uint32_t *value) {
  *value = 0;
  if (!pin || strlen(pin) != 6 || pin[0] == '0') return false;
  for (int i = 0; i < 6; i++) {
    if (pin[i] < '0' || pin[i] > '9') return false;
    *value = *value * 10 + pin[i] - '0';
  }
  return true;
}
void EncryptionPINRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *, mg_str args) {
  if (!Authorized(ri)) return;
  char *pin = nullptr;
  json_scanf(args.p, args.len, "{pin:%Q}", &pin);
  mgos::ScopedCPtr pin_owner(pin);
  uint32_t value;
  if (state != "waiting_encryption_pin" || conn == BLE_HS_CONN_HANDLE_NONE) {
    mg_rpc_send_errorf(ri, 409, "No device is waiting for an encryption PIN");
    return;
  }
  if (!ParsePIN(pin, &value) || value == passkey.load()) {
    mg_rpc_send_errorf(ri, 400, "Enter a six-digit PIN different from the current PIN");
    return;
  }
  new_pin = value;
  key_read_attempts = 0;
  deadline = mgos_uptime() + 90;
  SetPIN();
  mg_rpc_send_responsef(ri, "{}");
}
void StatusRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *, mg_str) {
  std::string list = "[";
  for (const auto &candidate : candidates) {
    if (mgos_uptime() - candidate.seen > 15) continue;
    char mac[MGOS_BT_ADDR_STR_LEN];
    mgos_bt_addr_to_str(&candidate.addr, 0, mac);
    if (list.size() > 1) list += ",";
    list += mgos::JSONPrintStringf("{mac:%Q,rssi:%d,encrypted:%B,model:%d}", mac,
                                   candidate.rssi, candidate.encrypted, candidate.model);
  }
  list += "]";
  mg_rpc_send_responsef(
      ri, "{state:%Q,error:%Q,role:%d,remaining_s:%d,candidates:%s}",
      state.c_str(), error.c_str(), role,
      Active() ? int(deadline - mgos_uptime()) : 0, list.c_str());
}
void DiscoverRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *, mg_str args) {
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
  int model = 0;
  json_scanf(args.p, args.len, "{model:%d}", &model);
  if (model != 0 && model != 3 && model != 0x01aa && model != 0x055b) {
    mg_rpc_send_errorf(ri, 400, "Unsupported thermometer model");
    return;
  }
  discovery_model = model;
  XiaomiGATTCancel();  // Discovery has priority over background thermometer reads.
  state = "scanning";
  error.clear();
  diagnostics.clear();
  role = -1;
  candidates.clear();
  deadline = mgos_uptime() + 60;
  mgos_bt_gap_scan_stop();
  mg_rpc_send_responsef(ri, "{}");
}
void StoreXiaomi(int selected_role, const Candidate &candidate) {
  role = selected_role;
  peer = candidate.addr;
  SecureRecord record = {};
  record.version = 1;
  record.protocol = kProtocolXiaomi;
  memcpy(record.mac, peer.addr, 6);
  if (!SaveRecord(role, record)) {
    Finish("Cannot save thermometer assignment");
    return;
  }
  char mac[MGOS_BT_ADDR_STR_LEN];
  mgos_bt_addr_to_str(&peer, 0, mac);
  if (role == 3) mgos_sys_config_set_blu_indoor_mac(mac);
  else mgos_sys_config_set_blu_outdoor_mac(mac);
  if (!mgos_sys_config_save(&mgos_sys_config, false, nullptr)) {
    Finish("Cannot save thermometer assignment; add it again");
    return;
  }
  ResetReadings();
  state = "waiting_beacon";
  deadline = mgos_uptime() + 60;
}
void PairRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *,
             mg_str args) {
  if (!Authorized(ri)) return;
  char *mac = nullptr, *pin = nullptr, *current_pin = nullptr;
  int selected_role = -1;
  json_scanf(args.p, args.len, "{role:%d,mac:%Q,pin:%Q,current_pin:%Q}",
             &selected_role, &mac, &pin, &current_pin);
  mgos::ScopedCPtr mac_owner(mac), pin_owner(pin), current_owner(current_pin);
  if (state != "scanning" || selected_role < 0 || selected_role > 5 ||
      !ValidMAC(mac)) {
    mg_rpc_send_errorf(
        ri, 400,
        "Select a discovered device and role");
    return;
  }
  uint32_t value = 0;
  // Compatibility for older panels: only an explicitly supplied PIN rotates
  // encryption. Normal pairing now keeps the sensor's existing BTHome key.
  if (pin && *pin && !ParsePIN(pin, &value)) {
    mg_rpc_send_errorf(ri, 400, "Invalid encryption PIN");
    return;
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
    mg_rpc_send_errorf(ri, 400, "Device expired; scan and wake it again");
    return;
  }
  if ((discovery_model && candidate->model != discovery_model) ||
      (selected_role == 5 ? candidate->model != 1 : selected_role >= 3 ?
      (candidate->model != 3 && candidate->model != 0x01aa && candidate->model != 0x055b) :
      candidate->model != (selected_role == 2 ? 5 : 2))) {
    mg_rpc_send_errorf(ri, 400, "Select the correct device model for this role");
    return;
  }
  for (int i = 0; i < 6; i++) {
    const char *other = SensorMAC(i);
    if (i != selected_role && other && strcasecmp(mac, other) == 0) {
      mg_rpc_send_errorf(ri, 400, "Device is already assigned to another role");
      return;
    }
  }
  if (candidate->model == 0x055b) {
    role = selected_role;
    peer = candidate->addr;
    const auto address = peer;
    const unsigned current = ++generation;
    diagnostics.clear();
    state = "reading_xiaomi";
    deadline = mgos_uptime() + 12;
    const bool started = XiaomiGATTStart(role, peer,
        [selected_role, address, current](const Sensor &reading, int voltage, const std::string &failure) {
      if (generation != current || state != "reading_xiaomi") return;
      if (!failure.empty()) { Finish(failure.c_str()); return; }
      // Keep the previous assignment until the model and measurements verify.
      const std::string previous = SensorMAC(selected_role) ? SensorMAC(selected_role) : "";
      char mac[MGOS_BT_ADDR_STR_LEN];
      mgos_bt_addr_to_str(&address, 0, mac);
      auto set_mac = [selected_role](const char *value) {
        if (selected_role == 3) mgos_sys_config_set_blu_indoor_mac(value);
        else mgos_sys_config_set_blu_outdoor_mac(value);
      };
      set_mac(mac);
      if (!mgos_sys_config_save(&mgos_sys_config, false, nullptr)) {
        set_mac(previous.c_str());
        Finish("Cannot save thermometer assignment");
        return;
      }
      SecureRecord record = {};
      record.version = 1;
      record.protocol = kProtocolXiaomiGATT;
      memcpy(record.mac, address.addr, 6);
      if (!SaveRecord(selected_role, record)) {
        set_mac(previous.c_str());
        mgos_sys_config_save(&mgos_sys_config, false, nullptr);
        Finish("Cannot save thermometer assignment; previous assignment retained");
        return;
      }
      SetThermometerReading(selected_role, address, reading, voltage);
      Finish(nullptr);
      mgos::InvokeCB([] { RestartService(); });
    });
    if (!started) Finish("Wait until the garage stops, then retry adding the thermometer");
    mg_rpc_send_responsef(ri, "{}");
    return;
  }
  if (candidate->model == 0x01aa) {
    StoreXiaomi(selected_role, *candidate);
    mg_rpc_send_responsef(ri, "{}");
    return;
  }
  uint32_t old_value = 0;
  if (candidate->encrypted) {
    if (current_pin == nullptr || strlen(current_pin) != 6 ||
        current_pin[0] == '0') {
      mg_rpc_send_errorf(ri, 400,
                         "Enter the current PIN of this secured device, or "
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
    if (value && old_value == value) {
      mg_rpc_send_errorf(
          ri, 400, "Choose a different new PIN to rotate the encryption key");
      return;
    }
  }
  attempt_started = mgos_uptime();
  diagnostics.clear();
  role = selected_role;
  peer = candidate->addr;
  existing = candidate->encrypted;
  mgos_bt_addr_to_esp32(&peer, &native_peer);
  saved_io = ble_hs_cfg.sm_io_cap;
  saved_bonding = ble_hs_cfg.sm_bonding;
  saved_mitm = ble_hs_cfg.sm_mitm;
  saved_our_keys = ble_hs_cfg.sm_our_key_dist;
  saved_their_keys = ble_hs_cfg.sm_their_key_dist;
  security_changed = true;
  // Only peers advertising BLE authentication require PIN bonding. Newer
  // firmware uses the PIN for payload encryption configuration instead.
  ble_hs_cfg.sm_io_cap = existing ? BLE_SM_IO_CAP_KEYBOARD_ONLY
                                  : BLE_SM_IO_CAP_NO_IO;
  ble_hs_cfg.sm_bonding = 1;
  // bt-common leaves both at NimBLE's zero defaults. Bonding also needs
  // encryption-key distribution, particularly for legacy SMP peers.
  ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC;
  ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC;
  ble_hs_cfg.sm_mitm = existing ? 1 : 0;
  Trace(mgos::SPrintf("secured=%d sc=%d keys=%d/%d", existing,
                      ble_hs_cfg.sm_sc, ble_hs_cfg.sm_our_key_dist,
                      ble_hs_cfg.sm_their_key_dist));
  Trace(mgos::SPrintf("role=%d addr_type=%d age=%.1fs rssi=%d", role,
                      native_peer.type, mgos_uptime() - candidate->seen, candidate->rssi));
  connection_attempts = key_read_attempts = empty_key_reads = 0;
  security_bond_resume = false;
  security_pairing_started = security_retry_used = false;
  bond_renewal = BondRenewal::None;
  key_reconnect_used = false;
  key_provision_used = false;
  pending_key.fill(0);
  new_pin = value;
  rotated = false;
  passkey.store(existing ? old_value : 0);
  deadline = mgos_uptime() + 90;
  generation++;
  SecureRecord saved;
  if (!new_pin && LoadRecord(role, &saved) && saved.protocol == kProtocolBTHome &&
      memcmp(saved.mac, peer.addr, 6) == 0) {
    state = "verifying_existing_key";
    deadline = mgos_uptime() + 3;
    Trace("Checking existing encrypted readings before BLE connection");
  } else {
    Connect();
  }
  mg_rpc_send_responsef(ri, "{}");
}
void CancelRPC(mg_rpc_request_info *ri, void *, mg_rpc_frame_info *, mg_str) {
  if (!Authorized(ri)) return;
  Finish("Pairing cancelled");
  mg_rpc_send_responsef(ri, "{}");
}
}  // namespace

bool PairingScan() {
  return state == "scanning" || state == "waiting_encrypted_beacon" || state == "waiting_beacon" ||
         state == "waiting_pairing_mode" || state == "waiting_security_retry" ||
         state == "waiting_user_pairing" || state == "verifying_existing_key";
}
bool PairingConnecting() {
  return XiaomiGATTConnecting() || (Active() && state != "reading_xiaomi" && !PairingScan());
}
bool PairingBusy() {
  return Active() && state != "reading_xiaomi";
}
void PairingVerified(int verified_role) {
  if ((state == "waiting_encrypted_beacon" || state == "waiting_beacon" ||
       state == "verifying_existing_key") && verified_role == role) {
    Finish(nullptr);
    // A newly verified optional sensor changes the HomeKit service list.
    // Rebuild outside the scan callback and increment HomeKit's config number.
    if (verified_role >= 2) mgos::InvokeCB([] { RestartService(); });
  }
}
void PairingAdvertisement(void *data) {
  if (state != "scanning" && state != "waiting_pairing_mode" &&
      state != "waiting_security_retry" && state != "waiting_user_pairing") return;
  auto *result = static_cast<mgos_bt_gap_scan_result *>(data);
  bool pairing = false, encrypted = false;
  uint16_t model = 0;
  Reading reading;
  if (state == "scanning" && discovery_model == 0x01aa &&
      (DecodeXiaomiAdvertisement(
           reinterpret_cast<const uint8_t *>(result->adv_data.p),
           result->adv_data.len, result->addr.addr, &reading) ||
       DecodeXiaomiAdvertisement(
           reinterpret_cast<const uint8_t *>(result->scan_rsp.p),
           result->scan_rsp.len, result->addr.addr, &reading))) {
    model = 0x01aa;
    pairing = true;
  } else if (state == "scanning" && discovery_model == 0x055b &&
      (Xiaomi03Advertisement(reinterpret_cast<const uint8_t *>(result->adv_data.p), result->adv_data.len) ||
       Xiaomi03Advertisement(reinterpret_cast<const uint8_t *>(result->scan_rsp.p), result->scan_rsp.len))) {
    model = 0x055b;
    pairing = true;
  } else {
  mg_str manufacturer = mgos_bt_gap_parse_adv_data(
        result->scan_rsp, MGOS_BT_GAP_EIR_MANUFACTURER_SPECIFIC_DATA);
    if (manufacturer.len < 2 || uint8_t(manufacturer.p[0]) != 0xa9 ||
        uint8_t(manufacturer.p[1]) != 0x0b)
      return;
    for (size_t i = 2; i < manufacturer.len;) {
      uint8_t type = manufacturer.p[i++];
      size_t n = type == 1 || type == 0x0b ? 2 : type == 0x0a ? 6 : 0;
      if (!n || n > manufacturer.len - i) return;
      if (type == 1) {
        pairing = manufacturer.p[i] & 0x10;
        encrypted = manufacturer.p[i] & 2;
      }
      if (type == 0x0b)
        model = uint8_t(manufacturer.p[i]) | (uint16_t(uint8_t(manufacturer.p[i + 1])) << 8);
      i += n;
    }
  }
  if (!pairing || (model != 1 && model != 2 && model != 5 && model != 3 && model != 0x01aa && model != 0x055b) ||
      (state == "scanning" && discovery_model && model != discovery_model)) return;
  if (state == "waiting_pairing_mode" || state == "waiting_security_retry" ||
      state == "waiting_user_pairing") {
    if (memcmp(peer.addr, result->addr.addr, 6) == 0) {
      peer = result->addr;
      mgos_bt_addr_to_esp32(&peer, &native_peer);
      if (rotated) ConfigureReconnectAuthentication(encrypted);
      if (bond_renewal == BondRenewal::WaitingForPairingMode)
        bond_renewal = BondRenewal::Ready;
      Connect();
    }
    return;
  }
  for (auto &candidate : candidates) {
    if (memcmp(candidate.addr.addr, result->addr.addr, 6) == 0) {
      candidate.addr = result->addr;
      candidate.rssi = result->rssi;
      candidate.encrypted = encrypted;
      candidate.model = model;
      candidate.seen = mgos_uptime();
      return;
    }
  }
  if (candidates.size() < 8)
    candidates.push_back(
        {result->addr, result->rssi, encrypted, model, mgos_uptime()});
}
void PairingTimeout() {
  if (!Active() || mgos_uptime() < deadline) return;
  if (state == "verifying_existing_key") {
    state = "waiting_pairing_mode";
    deadline = mgos_uptime() + 60;
    Trace("No verified beacon yet; waiting for fresh pairing advertisement");
  } else if (state == "waiting_security_retry")
    WaitForSecurityRetryWake();
  else if (state == "securing_link" && connection_attempts < 3)
    WaitForUserPairing();
  else
    Finish("Pairing timed out; wake the device and retry");
}
void PairingInit() {
  auto *rpc = mgos_rpc_get_global();
  mg_rpc_add_handler(rpc, "Shelly.BLU.Discover", "{model:%d}", DiscoverRPC, nullptr);
  mg_rpc_add_handler(rpc, "Shelly.BLU.Pair",
                     "{role:%d,mac:%Q,pin:%Q,current_pin:%Q}", PairRPC,
                     nullptr);
  mg_rpc_add_handler(rpc, "Shelly.BLU.Status", "", StatusRPC, nullptr);
  mg_rpc_add_handler(rpc, "Shelly.BLU.Cancel", "", CancelRPC, nullptr);
  mg_rpc_add_handler(rpc, "Shelly.BLU.Resume", "", ResumeRPC, nullptr);
  mg_rpc_add_handler(rpc, "Shelly.BLU.SetPIN", "{pin:%Q}", EncryptionPINRPC, nullptr);
  mgos_set_timer(
      500, MGOS_TIMER_REPEAT,
      [](void *) { PairingTimeout(); },
      nullptr);
}
}  // namespace blu
}  // namespace shelly
#endif
