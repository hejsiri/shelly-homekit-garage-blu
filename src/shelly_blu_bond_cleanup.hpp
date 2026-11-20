#pragma once
#include <initializer_list>
// Include after NimBLE host/ble_store.h. Keep deletion bounded even when a
// store backend reports success without removing the requested record.
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
extern "C" {
struct ble_hs_dev_records;
struct ble_hs_dev_records *ble_rpa_find_peer_dev_rec(uint8_t *addr);
int ble_hs_resolv_list_rmv(uint8_t type, uint8_t *addr);
int ble_rpa_remove_peer_dev_rec(struct ble_hs_dev_records *record);
}
#endif
namespace shelly { namespace blu {
inline int DeleteTransportBond(const ble_addr_t &peer) {
  for (int type : {BLE_STORE_OBJ_TYPE_OUR_SEC, BLE_STORE_OBJ_TYPE_PEER_SEC,
                   BLE_STORE_OBJ_TYPE_CCCD}) {
    union ble_store_key query = {};
    if (type == BLE_STORE_OBJ_TYPE_CCCD) query.cccd.peer_addr = peer;
    else query.sec.peer_addr = peer;
    const int limit = type == BLE_STORE_OBJ_TYPE_CCCD ?
        MYNEWT_VAL(BLE_STORE_MAX_CCCDS) : MYNEWT_VAL(BLE_STORE_MAX_BONDS);
    for (int removed = 0;; ++removed) {
      union ble_store_value value = {};
      int result = ble_store_read(type, &query, &value);
      if (result == BLE_HS_ENOENT) break;
      if (result != 0) return result;
      if (removed >= limit) return BLE_HS_ESTORE_FAIL;
      union ble_store_key exact = {};
      ble_store_key_from_value(type, &exact, &value);
      result = ble_store_delete(type, &exact);
      if (result != 0) return result;
    }
  }
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
  // Same peer-specific privacy cleanup as NimBLE, after storage is verified.
  ble_addr_t identity = peer;
  auto *record = ble_rpa_find_peer_dev_rec(identity.val);
  if (record) {
    (void) ble_hs_resolv_list_rmv(identity.type, identity.val);
    return ble_rpa_remove_peer_dev_rec(record);
  }
#endif
  return 0;
}
}}  // namespace shelly::blu
