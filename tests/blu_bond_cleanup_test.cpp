#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#define MYNEWT_VAL(x) MYNEWT_VAL_##x
#define MYNEWT_VAL_BLE_HOST_BASED_PRIVACY 1
#define MYNEWT_VAL_BLE_STORE_MAX_CCCDS 8
#define MYNEWT_VAL_BLE_STORE_MAX_BONDS 3
constexpr int BLE_STORE_OBJ_TYPE_OUR_SEC=1, BLE_STORE_OBJ_TYPE_PEER_SEC=2,
 BLE_STORE_OBJ_TYPE_CCCD=3, BLE_HS_ENOENT=5, BLE_HS_ESTORE_FAIL=9;
struct ble_addr_t { uint8_t type; uint8_t val[6]; };
struct Key {ble_addr_t peer_addr; int id;};
union ble_store_key {Key sec,cccd;};
union ble_store_value {Key sec,cccd;};
struct Entry {int type; Key key;};
std::vector<Entry> entries;
int reads=0,deletes=0,privacy=0,remove_result=0,read_result=0;
bool stuck=false;
bool Equal(const ble_addr_t&a,const ble_addr_t&b){return a.type==b.type&&!memcmp(a.val,b.val,6);}
int ble_store_read(int type,const ble_store_key*k,ble_store_value*v){
 ++reads;if(read_result)return read_result;
 for(auto&e:entries)if(e.type==type&&Equal(e.key.peer_addr,k->sec.peer_addr)){v->sec=e.key;return 0;}
 return BLE_HS_ENOENT;
}
void ble_store_key_from_value(int,ble_store_key*k,const ble_store_value*v){k->sec=v->sec;}
int ble_store_delete(int type,const ble_store_key*k){
 ++deletes;if(remove_result)return remove_result;
 for(auto i=entries.begin();i!=entries.end();++i)if(i->type==type&&Equal(i->key.peer_addr,k->sec.peer_addr)&&i->key.id==k->sec.id){if(!stuck)entries.erase(i);return 0;}
 return BLE_HS_ENOENT;
}
#include "shelly_blu_bond_cleanup.hpp"
struct ble_hs_dev_records {} record;
extern "C" ble_hs_dev_records *ble_rpa_find_peer_dev_rec(uint8_t*){return &record;}
extern "C" int ble_hs_resolv_list_rmv(uint8_t,uint8_t*){++privacy;return 0;}
extern "C" int ble_rpa_remove_peer_dev_rec(ble_hs_dev_records*r){assert(r==&record);++privacy;return 0;}
void Reset(){entries.clear();reads=deletes=privacy=read_result=remove_result=0;stuck=false;}
int main(){
 ble_addr_t target={0,{1,2,3,4,5,6}},other={1,{1,2,3,4,5,6}};
 Reset();assert(shelly::blu::DeleteTransportBond(target)==0&&deletes==0&&reads==3&&privacy==2);
 Reset();for(int t:{1,2,3})for(int i=0;i<(t==3?8:3);++i){entries.push_back({t,{target,i}});entries.push_back({t,{other,i}});}
 assert(shelly::blu::DeleteTransportBond(target)==0&&deletes==14&&entries.size()==14&&privacy==2);
 for(auto&e:entries)assert(Equal(e.key.peer_addr,other));
 Reset();entries.push_back({1,{target,0}});stuck=true;
 assert(shelly::blu::DeleteTransportBond(target)==BLE_HS_ESTORE_FAIL&&deletes==3&&reads==4&&privacy==0);
 Reset();entries.push_back({3,{target,0}});stuck=true;
 assert(shelly::blu::DeleteTransportBond(target)==BLE_HS_ESTORE_FAIL&&deletes==8&&privacy==0);
 Reset();read_result=77;assert(shelly::blu::DeleteTransportBond(target)==77&&deletes==0&&privacy==0);
 Reset();entries.push_back({1,{target,0}});remove_result=78;assert(shelly::blu::DeleteTransportBond(target)==78&&deletes==1&&privacy==0);
}
