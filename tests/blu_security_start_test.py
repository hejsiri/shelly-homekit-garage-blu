#!/usr/bin/env python3
"""Exercise production security scheduling without a Bluetooth controller."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/shelly_blu_pairing.cpp').read_text()
start=source.index('bool ReusablePeerBond() {')
end=source.index('\nvoid ReadKey();',start)
enc_start=source.index('    ble_gap_conn_desc desc = {};',source.index('  } else if (event->type == BLE_GAP_EVENT_ENC_CHANGE) {'))
enc_end=source.index('  } else if (event->type == BLE_GAP_EVENT_DISCONNECT) {',enc_start)
wait_start=source.index('void WaitForUserPairing() {')
wait_end=source.index('void ResumeRPC(',wait_start)
retry_start=source.index('bool ResumeSecurityAfterDisconnect(')
retry_end=source.index('bool ResumeKeyAfterDisconnect()',retry_start)
disc_start=source.index('    const int reason =',enc_end)
disc_end=source.index('\n  }\n  return 0;',disc_start)
timeout_start=source.index('void PairingTimeout()')
timeout_end=source.index('void PairingInit()',timeout_start)
harness=r'''
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#define BLE_HS_CONN_HANDLE_NONE 0xffff
#define BLE_HS_EALREADY 2
#define BLE_HS_ENOENT 5
#define BLE_ERR_REM_USER_CONN_TERM 0x13
#define BLE_HS_ERR_HCI_BASE 0x200
#define LOG(...) ((void)0)
struct ble_gap_conn_desc { int peer_id_addr=42; struct { bool encrypted=false,bonded=false,authenticated=false; } sec_state; } description;
struct ble_store_key_sec { int peer_addr=0; };
struct ble_store_value_sec { bool ltk_present=false,authenticated=false; } stored_bond;
int bond_result=BLE_HS_ENOENT,pair_requests=0;
bool security_pair_required=false;
bool security_bond_resume=false;
bool security_pairing_started=false,security_retry_used=false;
enum class BondRenewal { None, WaitingForPairingMode, Ready, Used };
BondRenewal bond_renewal=BondRenewal::None;
int renewals=0,terminations=0;
bool renewal_result=true;
unsigned security_ticket=0;
unsigned generation=1;
unsigned connection_attempts=1;
int role=2,conn=1,lookup=0,security_result=0,requests=0,discoveries=0;
bool RenewSelectedBond(int handle) { assert(handle==conn);renewals++;return renewal_result; }
int ble_gap_terminate(int,int) { terminations++;return 0; }
double deadline=0;
double now=100;
double mgos_uptime(){ return now; }
bool ResumeKeyAfterDisconnect() { return false; }
bool existing=false;
int discovery_model=0;
std::string state="idle",error;
struct Timer { void (*cb)(void *); void *arg; };
std::vector<Timer> timers;
namespace mgos {
template<typename F> void InvokeCB(F callback) { callback(); }
std::string SPrintf(const char *fmt,...) {
 char b[256];va_list a;va_start(a,fmt);vsnprintf(b,sizeof(b),fmt,a);va_end(a);return b;
}}
bool Active() { return state!="idle" && state!="error" && state!="done"; }
void Trace(const std::string &) {}
void Finish(const char *message) { state="error";error=message;security_ticket++; }
void FailStart(int result,const char *message) { if(result)Finish(message); }
void Discover() { discoveries++;state="discovering_services"; }
int ble_gap_conn_find(int,ble_gap_conn_desc *out) { if(!lookup)*out=description;return lookup; }
int ble_gap_security_initiate(int) { requests++;return security_result; }
int ble_gap_pair_initiate(int) { pair_requests++;return security_result; }
int ble_store_read_peer_sec(const ble_store_key_sec *key,ble_store_value_sec *out) {
 assert(key->peer_addr==42);*out=stored_bond;return bond_result;
}
int mgos_set_timer(int delay,int flags,void (*cb)(void *),void *arg) {
 assert(delay==500 && flags==0);timers.push_back({cb,arg});return 1;
}
'''
harness+=source[start:end]+source[retry_start:retry_end]+source[wait_start:wait_end]+source[timeout_start:timeout_end]+r'''
void Disconnect(int event_reason,unsigned current=generation) {
 struct { struct { int reason; } disconnect; } event_data{{event_reason}};
 auto *event=&event_data;
'''+source[disc_start:disc_end]+r'''
}
void EncryptionChange(int event_status) {
 struct { struct { int status,conn_handle; } enc_change; } event_data{{event_status,1}};
 auto *event=&event_data;
 const unsigned current=generation;
'''+source[enc_start:enc_end]+r'''
}
void Reset() {
 security_ticket++;role=2;conn=1;lookup=security_result=requests=discoveries=0;
 existing=false;state="connecting";error.clear();description={};timers.clear();
 stored_bond={};bond_result=BLE_HS_ENOENT;pair_requests=0;security_pair_required=false;
 security_bond_resume=false;bond_renewal=BondRenewal::None;renewals=terminations=0;renewal_result=true;
 security_pairing_started=security_retry_used=false;now=100;connection_attempts=1;
}
void Tick() { auto pending=timers;timers.clear();for(auto &t:pending)t.cb(t.arg); }
int main() {
 // Reproduce the reported endpoint loop: a stored LTK exists, encryption
 // restore stalls, and the next confirmed pairing window renews only once.
 Reset();role=1;bond_result=0;stored_bond.ltk_present=true;
 ScheduleSecurity();assert(security_bond_resume && requests==1 && pair_requests==0);
 WaitForUserPairing();
 assert(state=="waiting_user_pairing" && bond_renewal==BondRenewal::WaitingForPairingMode);
 assert(renewals==0 && pair_requests==0 && terminations==1);
 assert(ResumePairingWindow() && state=="waiting_pairing_mode");
 assert(bond_renewal==BondRenewal::WaitingForPairingMode && renewals==0);
 // Production advertisement handling arms Ready only for the selected peer.
 bond_renewal=BondRenewal::Ready;conn=1;ScheduleSecurity();
 assert(renewals==1 && pair_requests==1 && requests==1 && !security_bond_resume);
 assert(bond_renewal==BondRenewal::Used && state=="securing_link");
 WaitForUserPairing();assert(state=="error" && renewals==1 && pair_requests==1);
 assert(error.find("renewed BLE pairing")!=std::string::npos);
 // Reported blu29 sequence: fresh SMP starts, then the peripheral closes
 // with HCI 0x13. Keep the attempt alive for one fresh-advertisement reconnect.
 Reset();bond_renewal=BondRenewal::Ready;ScheduleSecurity();
 const unsigned previous_generation=generation;
 Disconnect(0x213);
 assert(conn==BLE_HS_CONN_HANDLE_NONE && generation==previous_generation+1);
 assert(state=="waiting_security_retry" && security_retry_used && deadline==105);
 assert(renewals==1 && pair_requests==1 && terminations==0);
 PairingTimeout();assert(state=="waiting_security_retry");
 now=105;PairingTimeout();assert(state=="waiting_user_pairing" && deadline==285);
 assert(ResumePairingWindow());conn=1;ScheduleSecurity();Tick();
 assert(renewals==1 && requests==1 && security_pairing_started);
 Disconnect(0x213);assert(state=="error" && renewals==1);
 // If the old host bond was already removed by the previous attempt, a new
 // pairing can take the same bounded recovery path, without any bond deletion.
 Reset();ScheduleSecurity();Tick();Disconnect(0x213);
 assert(state=="waiting_security_retry" && renewals==0);
 conn=1;ScheduleSecurity();Tick();
 description.sec_state.encrypted=description.sec_state.bonded=true;
 EncryptionChange(0);assert(state=="discovering_services" && discoveries==1);
 Reset();ScheduleSecurity();Tick();Disconnect(0x213);
 conn=1;ScheduleSecurity();Tick();now=115;PairingTimeout();
 assert(state=="error" && security_retry_used && requests==2);
 // A stale disconnect or a delayed security timer cannot restart cancelled work.
 Reset();ScheduleSecurity();Tick();unsigned cancelled=generation++;
 state="error";Disconnect(0x213,cancelled);assert(!security_retry_used);
 Reset();ScheduleSecurity();Disconnect(0x213);Tick();
 assert(state=="error" && requests==0 && !security_retry_used);
 // No retry for LTK restore, timeout/local termination or an SMP rejection.
 Reset();bond_result=0;stored_bond.ltk_present=true;ScheduleSecurity();
 Disconnect(0x213);assert(state=="error" && !security_retry_used);
 for(int reason : {0x208,0x216,0x505}) {
  Reset();ScheduleSecurity();Tick();Disconnect(reason);
  assert(state=="error" && !security_retry_used);
 }
 Reset();ScheduleSecurity();Tick();EncryptionChange(0x505);Disconnect(0x213);
 assert(state=="error" && !security_retry_used);
 // Storage failure stops renewal; fresh requests still require encrypted bonds.
 Reset();bond_renewal=BondRenewal::Ready;renewal_result=false;
 ScheduleSecurity();assert(state=="error" && requests==0 && pair_requests==0 && renewals==1);
 Reset();bond_renewal=BondRenewal::Ready;security_result=BLE_HS_EALREADY;
 ScheduleSecurity();assert(state=="securing_link" && pair_requests==1 && discoveries==0);
 EncryptionChange(0);assert(state=="error" && discoveries==0);
 // A timeout without a stored LTK never marks an unrelated bond for deletion.
 Reset();ScheduleSecurity();Tick();WaitForUserPairing();
 assert(bond_renewal==BondRenewal::None && renewals==0 && state=="waiting_user_pairing");
 // A completed GAP event accepts only the required encrypted/bonded/authenticated states.
 for (bool require_pin : {false,true}) for(int flags=0;flags<8;flags++) {
  Reset();existing=require_pin;state="securing_link";
  description.sec_state.encrypted=flags&1;
  description.sec_state.bonded=flags&2;
  description.sec_state.authenticated=flags&4;
  EncryptionChange(0);
  const bool allowed=(flags&3)==3 && (!require_pin || (flags&4));
  assert(discoveries==int(allowed));
  assert(state==(allowed?"discovering_services":"error"));
 }
 // Failure diagnostics query the real link, instead of short-circuiting to all zeros.
 Reset();state="securing_link";
 description.sec_state.encrypted=description.sec_state.bonded=description.sec_state.authenticated=true;
 EncryptionChange(0x505);
 assert(state=="error" && discoveries==0);
 assert(error.find("encrypted=1, bonded=1, authenticated=1")!=std::string::npos);
 Reset();state="securing_link";lookup=7;EncryptionChange(0);
 assert(state=="error" && error.find("lookup=0x7")!=std::string::npos);
 // A saved usable bond starts encryption before a peripheral can request new pairing.
 for (int sensor : {0,1,2,3,4,5}) {
  Reset();role=sensor;bond_result=0;stored_bond.ltk_present=true;
  ScheduleSecurity();assert(requests==1 && pair_requests==0 && timers.empty());
 }
 // A record without an LTK must not be treated as a reusable bond.
 Reset();bond_result=0;ScheduleSecurity();assert(requests==0 && timers.size()==1);
 // Existing legacy authentication remains mandatory; upgrade a weak bond explicitly.
 Reset();existing=true;bond_result=0;stored_bond.ltk_present=true;
 ScheduleSecurity();Tick();assert(requests==0 && pair_requests==1 && discoveries==0);
 description.sec_state.encrypted=description.sec_state.bonded=true;
 StartSecurity();assert(discoveries==0 && pair_requests==2);
 Reset();existing=true;bond_result=0;stored_bond.ltk_present=stored_bond.authenticated=true;
 ScheduleSecurity();assert(requests==1 && pair_requests==0 && timers.empty());
 // A storage error must not trust partially populated values.
 Reset();stored_bond.ltk_present=stored_bond.authenticated=true;
 ScheduleSecurity();assert(requests==0 && timers.size()==1);
 Reset();ScheduleSecurity();assert(requests==0 && timers.size()==1);Tick();assert(requests==1 && state=="securing_link");
 // If Motion secures the link itself, do not issue another pairing request.
 Reset();ScheduleSecurity();description.sec_state.encrypted=description.sec_state.bonded=true;
 Tick();assert(requests==0 && discoveries==1);
 // In-progress pairing remains pending; it is not a failed PIN check.
 Reset();security_result=BLE_HS_EALREADY;ScheduleSecurity();Tick();
 assert(requests==1 && discoveries==0 && error.empty() && state=="securing_link");
 // Authenticated legacy peers still require MITM and a bond.
 Reset();existing=true;description.sec_state.encrypted=description.sec_state.bonded=true;
 ScheduleSecurity();Tick();assert(state=="error" && discoveries==0 && requests==0);
 Reset();existing=true;description.sec_state.encrypted=description.sec_state.bonded=description.sec_state.authenticated=true;
 ScheduleSecurity();Tick();assert(discoveries==1 && requests==0);
 Reset();description.sec_state.encrypted=true;ScheduleSecurity();Tick();assert(state=="error" && discoveries==0);
 // Cancellation, a completed ENC_CHANGE, and replacement connections invalidate old work.
 Reset();ScheduleSecurity();Finish("cancelled");Tick();assert(requests==0);
 Reset();ScheduleSecurity();Discover();Tick();assert(requests==0 && discoveries==1);
 Reset();ScheduleSecurity();ScheduleSecurity();Tick();assert(requests==1);
 // Button1 can initiate security first, just like Motion.
 Reset();role=5;ScheduleSecurity();assert(requests==0 && timers.size()==1);Tick();assert(requests==1);
 Reset();role=5;ScheduleSecurity();description.sec_state.encrypted=description.sec_state.bonded=true;
 Tick();assert(requests==0 && discoveries==1);
 Reset();role=5;existing=true;ScheduleSecurity();description.sec_state.encrypted=description.sec_state.bonded=true;
 Tick();assert(state=="error" && discoveries==0);
 // Door/Window allows peripheral-initiated security and has a bounded deadline.
 for (int endpoint : {0,1}) {
  Reset();role=endpoint;ScheduleSecurity();assert(requests==0 && timers.size()==1 && deadline==115);
  Tick();assert(requests==1);
 }
 // H&T keeps immediate security start.
 Reset();role=3;ScheduleSecurity();assert(requests==1 && timers.empty());
 Reset();lookup=7;ScheduleSecurity();Tick();assert(state=="error" && error.find("0x7")!=std::string::npos && requests==0);
}
'''
with tempfile.TemporaryDirectory(prefix='shelly-security-start-') as temp:
 d=Path(temp);(d/'test.cpp').write_text(harness)
 subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(d/'test.cpp'),'-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],check=True)
print('PASS: stored bond resumption, legacy authentication upgrade, pending SMP, cancellation and security validation')
