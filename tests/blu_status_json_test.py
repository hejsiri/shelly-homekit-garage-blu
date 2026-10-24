#!/usr/bin/env python3
"""Compile the production status serializer against the firmware's real Frozen.

Python's strict JSON parser catches unquoted dynamic keys that Frozen accepts.
Run after a firmware build has populated the local Mongoose OS dependencies.
"""
import json
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
mos = Path(os.environ.get('SHELLY_MOS_SOURCE', str(Path.home() / '.mos/work/shelly-garage-blu/deps/modules/mongoose-os')))
frozen = mos / 'src/frozen'
source = (root / 'src/shelly_blu_garage.cpp').read_text()
start = source.index('std::string GetInfoJSON() {')
end = source.index('\n}  // namespace blu', start)
function = source[start:end]
harness = r'''
#include <string>
#include <cstdarg>
#include <iostream>
#include "frozen.h"
namespace mgos {
std::string JSONPrintStringf(const char *fmt, ...) {
  std::string result;
  struct json_out out = {};
  out.u.data = reinterpret_cast<char *>(&result);
  out.printer = [](struct json_out *o, const char *p, size_t n) -> int {
    reinterpret_cast<std::string *>(o->u.data)->append(p, n); return n;
  };
  va_list ap; va_start(ap, fmt); json_vprintf(&out, fmt, ap); va_end(ap);
  return result;
}
}
struct Sensor {
 bool seen = false, active = false;
 int battery = -1, rssi = 0, temperature10 = -1000, humidity = -1;
 double last_seen = 0;
} sensors[6];
struct Address { unsigned char addr[6]; } addresses[6] = {};
bool known[6] = {true, true, true, true, false, true};
unsigned button_press_sequence = 0;
unsigned packet_sequence[6] = {1,2,3,4,5,6};
double button_press_time = 0;
bool Enabled() { return true; }
bool Ready() { return true; }
bool CarPresent() { return false; }
bool CarPresenceReady() { return true; }
const char *mgos_sys_config_get_blu_button_name() { return ""; }
const char *mgos_sys_config_get_blu_car_name() { return "Auto \"garaż\" <test>"; }
int mgos_sys_config_get_blu_presence_timeout_s() { return 45; }
int mgos_sys_config_get_blu_stale_after_s() { return 180; }
double mgos_uptime() { return 150; }
bool SecureFor(int role, const unsigned char *) { return role!=3; }
bool XiaomiContactFor(int role,const unsigned char*){return role==0;}
bool XiaomiFor(int role, const unsigned char *) { return role==3; }
bool XiaomiSecureFor(int,const unsigned char*){return false;}
int SensorTimeout(int){return 180;}
bool SensorPaired(int role) { return role!=4; }
bool SensorReady(int role) { return role!=4; }
bool HTReady(int role,bool) { return role==3; }
const char *SensorName(int) { return "Czujnik \"garaż\" <test>"; }
const char *SensorMAC(int role) { return role == 4 ? "" : "AA:BB:CC:DD:EE:FF"; }
'''
main = r'''
int main() {
 sensors[0].seen = sensors[0].active = true;
 sensors[0].battery = 76; sensors[0].rssi = -40; sensors[0].last_seen = 144;
 sensors[3].seen = true; sensors[3].temperature10 = -52; sensors[3].humidity = 65;
 std::cout << GetInfoJSON();
}
'''
with tempfile.TemporaryDirectory(prefix='shelly-status-json-') as temp:
    d = Path(temp)
    (d / 'test.cpp').write_text(harness + function + main)
    subprocess.run(['clang', '-I', str(frozen), '-c', str(frozen / 'frozen.c'), '-o', str(d / 'frozen.o')], check=True)
    subprocess.run(['clang++', '-std=c++14', '-I', str(frozen), str(d / 'test.cpp'), str(d / 'frozen.o'), '-o', str(d / 'test')], check=True)
    result = json.loads(subprocess.check_output([str(d / 'test')]))
    for role in ['closed', 'open', 'motion', 'indoor', 'outdoor', 'button']:
        assert isinstance(result[role + '_mac'], str)
        assert isinstance(result[role], dict)
        assert result[role]['name'] == 'Czujnik "garaż" <test>'
        assert result[role]['packet_seq'] > 0
    assert result['car_present'] is False
    assert result['car_presence_ready'] is True
    assert result['button_name'] == ''
    assert result['car_name'] == 'Auto "garaż" <test>'
    assert result['closed']['active'] is True
    assert result['closed']['protocol'] == 'xiaomi_contact'
    assert result['closed']['age_s'] == 6
    assert result['indoor']['paired'] is True and result['indoor']['secure'] is False
    assert result['indoor']['protocol']=='xiaomi' and result['indoor']['temperature_ready'] is True
    assert result['indoor']['temperature'] == -5.2
    assert result['indoor']['humidity'] == 65
    assert result['outdoor']['seen'] is False
    assert result['outdoor']['secure'] is False
    assert result['outdoor']['age_s'] == -1
print('PASS: production BLE status is strict JSON for all six roles, including unseen sensors')
