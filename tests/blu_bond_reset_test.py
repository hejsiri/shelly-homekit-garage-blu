#!/usr/bin/env python3
"""Verify bounded bond replacement on an explicit repeat-pairing event."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/shelly_blu_pairing.cpp').read_text()
a=source.index('bool RenewSelectedBond(uint16_t handle) {')
b=source.index('\nvoid Connect()',a)
harness=r'''
#include <cassert>
#include <atomic>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <string>
#define BLE_HS_ENOENT 5
struct Address { int id; };
struct ble_gap_conn_desc { Address peer_id_addr; };
std::atomic<bool> repeat_used(false);
std::atomic<unsigned> passkey(0);
bool existing=false,active=true;
uint16_t conn=7;
int calls=0,result=0,lookup=0,deleted=0;
bool Active() { return active; }
int ble_gap_conn_find(uint16_t h,ble_gap_conn_desc *d) { assert(h==conn);d->peer_id_addr.id=42;return lookup; }
int ble_store_util_delete_peer(Address *a) { calls++;deleted=a->id;return result; }
void Trace(const std::string &) {}
namespace mgos { std::string SPrintf(const char *fmt,...) {
 char b[128];va_list a;va_start(a,fmt);vsnprintf(b,sizeof(b),fmt,a);va_end(a);return b;
}}
'''
harness+=source[a:b]+r'''
void Reset() { repeat_used=false;passkey=0;existing=false;active=true;calls=result=lookup=deleted=0; }
int main() {
 Reset();assert(!RenewSelectedBond(8) && calls==0);
 active=false;assert(!RenewSelectedBond(7) && calls==0);
 Reset();existing=true;assert(!RenewSelectedBond(7) && calls==0);
 passkey=123456;assert(RenewSelectedBond(7) && calls==1 && deleted==42);
 assert(!RenewSelectedBond(7) && calls==1);
 // Modern peers without PIN authentication can renew their stale host bond.
 Reset();assert(RenewSelectedBond(7) && calls==1 && deleted==42);
 Reset();result=BLE_HS_ENOENT;assert(RenewSelectedBond(7));
 Reset();result=99;assert(!RenewSelectedBond(7));assert(!RenewSelectedBond(7) && calls==1);
 Reset();lookup=7;assert(!RenewSelectedBond(7) && calls==0);
}
'''
# Starting a new attempt must never erase an existing transport bond.
pair=source[source.index('void PairRPC('):source.index('void CancelRPC(')]
assert 'delete_peer' not in pair and 'ClearStaleBond' not in pair
with tempfile.TemporaryDirectory(prefix='shelly-bond-renew-') as temp:
 d=Path(temp);(d/'test.cpp').write_text(harness)
 subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(d/'test.cpp'),'-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],check=True)
print('PASS: bond preservation across attempts; explicit selected-connection renewal once; PIN policy and storage failure checks')
