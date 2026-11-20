// Compile the real NVS implementation; reset its RAM to simulate a restart.
#include <cassert>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include "shelly_blu_keys.cpp"
using namespace shelly::blu;
std::map<std::string,std::vector<uint8_t>> disk;
std::map<int,std::string> handles;
std::map<std::string,std::vector<uint8_t>> pending;
int next_handle=1,writes=0;bool fail_write=false;
esp_err_t nvs_open(const char *ns,int,nvs_handle_t *h){*h=next_handle++;handles[*h]=ns;return 0;}
std::string Key(nvs_handle_t h,const char *name){return handles[h]+":"+name;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *name,void *p,size_t *n){auto it=disk.find(Key(h,name));if(it==disk.end())return ESP_ERR_NVS_NOT_FOUND;if(*n<it->second.size())return 1;*n=it->second.size();memcpy(p,it->second.data(),*n);return 0;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *name,const void *p,size_t n){++writes;if(fail_write)return 1;pending[Key(h,name)]={(const uint8_t*)p,(const uint8_t*)p+n};return 0;}
esp_err_t nvs_erase_key(nvs_handle_t h,const char *name){disk.erase(Key(h,name));return 0;}
esp_err_t nvs_commit(nvs_handle_t){for(auto &p:pending)disk[p.first]=p.second;pending.clear();return 0;}
void nvs_close(nvs_handle_t h){handles.erase(h);}
void Reboot(){for(int i=0;i<2;++i){batteries[i]=BatteryCache();contacts[i]=ContactCache();loaded[i]=false;}}
int main(){
 const uint8_t mac[]={1,2,3,4,5,6},other[]={6,5,4,3,2,1};
 constexpr int64_t epoch=1800000000;
 SecureRecord record={};record.version=1;record.protocol=kProtocolMiBindkey;memcpy(record.mac,mac,6);
 assert(SaveRecord(0,record));
 CacheBattery(0,mac,60,epoch,10);
 int count=writes;
 for(int i=0;i<100;++i)assert(CachedBattery(0,mac,epoch+1,11)==60);
 assert(writes==count); // Status polling never repeatedly writes to flash.
 assert(CachedBattery(0,other,epoch+1,11)==-1);
 assert(CachedBattery(0,mac,epoch+86399,86409)==60);
 assert(CachedBattery(0,mac,epoch+86400,86410)==-1);
 // New value restarts the entire 24-hour lifetime, including identical values.
 CacheBattery(0,mac,42,epoch+100000,100000);
 CacheBattery(0,mac,41,epoch+100100,100100);
 assert(CachedBattery(0,mac,epoch+100100+86399,100100+86399)==41);
 CacheBattery(0,mac,41,epoch+200000,200000);
 Reboot();
 assert(CachedBattery(0,mac,0,2)==-1); // No reliable time after reboot yet.
 assert(CachedBattery(0,mac,epoch+200000+3600,5)==41); // Downtime counts.
 assert(CachedBattery(0,mac,epoch+200000+86400,5+82800)==-1);
 // A wall-clock correction during this boot cannot extend the lifetime.
 CacheBattery(0,mac,0,epoch+300000,100);
 assert(CachedBattery(0,mac,epoch+1,100+86399)==0);
 assert(CachedBattery(0,mac,epoch+1,100+86400)==-1);
 // A reading received before NTP is saved after synchronization, with its age.
 ForgetBattery(0);CacheBattery(0,mac,100,0,100);
 assert(!disk.count("blu_battery:bat_closed"));
 assert(CachedBattery(0,mac,0,150)==100);
 assert(CachedBattery(0,mac,epoch,200)==100);
 Reboot();assert(CachedBattery(0,mac,epoch+100,2)==100);
 assert(CachedBattery(0,mac,epoch+86300,86202)==-1);
 // Expired persisted data cannot be revived by a later backwards time jump.
 CacheBattery(0,mac,55,epoch,10);Reboot();
 assert(CachedBattery(0,mac,epoch+86400,3)==-1);
 assert(CachedBattery(0,mac,epoch+100,4)==-1);
 // Unknown timestamp, corrupt version/size, future timestamp and failed writes.
 BatteryRecord malformed;memcpy(malformed.mac,mac,6);malformed.percent=65;malformed.received_at=epoch+500;
 disk["blu_battery:bat_closed"]={(uint8_t*)&malformed,(uint8_t*)&malformed+sizeof(malformed)};
 Reboot();assert(CachedBattery(0,mac,epoch,3)==-1);
 malformed.version=99;disk["blu_battery:bat_closed"]={(uint8_t*)&malformed,(uint8_t*)&malformed+sizeof(malformed)};
 Reboot();assert(CachedBattery(0,mac,epoch+501,3)==-1);
 disk["blu_battery:bat_closed"]={1,2,3};Reboot();assert(CachedBattery(0,mac,epoch,3)==-1);
 ForgetBattery(0);fail_write=true;CacheBattery(0,mac,25,epoch,10);
 assert(CachedBattery(0,mac,epoch+1,11)==25);
 Reboot();assert(CachedBattery(0,mac,epoch+2,1)==-1);fail_write=false;
 CacheBattery(0,mac,54,epoch,10);ForgetBattery(0);Reboot();
 assert(CachedBattery(0,mac,epoch+1,1)==-1);
 // Scope is MCCGQ02HL endpoint roles only; other protocols remain independent.
 count=writes;CacheBattery(2,mac,80,epoch,10);CacheBattery(0,mac,101,epoch,10);CacheBattery(0,other,80,epoch,10);
 assert(writes==count);
 record.protocol=kProtocolQingpingBindkey;assert(SaveRecord(0,record));count=writes;
 CacheBattery(0,mac,80,epoch,10);assert(writes==count&&CachedBattery(0,mac,epoch,10)==-1);
 record.protocol=kProtocolMiBindkey;assert(SaveRecord(0,record));
 memcpy(record.mac,other,6);assert(SaveRecord(1,record));
 CacheBattery(0,mac,21,epoch,10);CacheBattery(1,other,82,epoch,10);
 Reboot();assert(CachedBattery(0,mac,epoch+100,2)==21);
 assert(CachedBattery(1,other,epoch+100,2)==82);
 ForgetBattery(0);Reboot();assert(CachedBattery(0,mac,epoch+200,2)==-1);
 assert(CachedBattery(1,other,epoch+200,2)==82);
 // Endpoint snapshots retain their real age; no expiry/refresh is invented at boot.
 CacheContact(0,mac,false,-64,epoch,100);Reboot();bool active;int signal;double age;
 assert(!CachedContact(0,mac,0,3,&active,&signal,&age));
 assert(CachedContact(0,mac,epoch+90,5,&active,&signal,&age)&&!active&&signal==-64&&age==90);
 int before=writes;
 assert(CachedContact(0,mac,epoch+100,15,&active,&signal,&age)&&age==100);
 assert(writes==before);
 assert(CachedContact(0,mac,epoch+86400,86315,&active,&signal,&age)&&age==86400);
 CacheContact(0,mac,true,-52,epoch+86401,86316);
 assert(CachedContact(0,mac,epoch+86402,86317,&active,&signal,&age)&&active&&age==1);
 Reboot();assert(CachedContact(0,mac,epoch+86405,2,&active,&signal,&age)&&active&&age==4);
 assert(!CachedContact(0,other,epoch+86405,2,&active,&signal,&age));
 ForgetContact(0);Reboot();assert(!CachedContact(0,mac,epoch+86410,2,&active,&signal,&age));
 CacheContact(0,mac,true,-52,0,10);
 assert(CachedContact(0,mac,epoch,20,&active,&signal,&age)&&age==10);
 Reboot();assert(CachedContact(0,mac,epoch+10,1,&active,&signal,&age)&&age==20);
 ForgetContact(0);
 puts("PASS: persisted Xiaomi contact state, original timestamp/downtime, replacement, no polling writes, clock sync, MAC isolation and removal");
 puts("PASS: 24-hour NVS battery cache, restart/downtime, replacement, clock sync/corrections, deletion, corruption, flash failure and MAC/protocol isolation");
}
