#!/usr/bin/env python3
"""Exercise the production HomeKit selection and post-verification refresh."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def extract(path, signature):
    source = (root / path).read_text()
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

selection = extract('src/shelly_hap_garage_door_opener.cpp', 'void GarageDoorOpener::AddBLUServices(')
verified = extract('src/shelly_blu_pairing.cpp', 'void PairingVerified(')
harness = r'''
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace mgos {
std::vector<std::function<void()>> callbacks;
void InvokeCB(std::function<void()> cb) { callbacks.push_back(cb); }
namespace hap {
struct Service { int role; bool temperature; };
struct Accessory { std::vector<Service *> services; void AddService(Service *s) { services.push_back(s); } };
}}
namespace shelly {
int restarts = 0;
void RestartService() { restarts++; }
namespace blu {
bool paired[6] = {};
bool SensorPaired(int role) { return paired[role]; }
std::unique_ptr<mgos::hap::Service> CreateMotionService() {
 return std::unique_ptr<mgos::hap::Service>(new mgos::hap::Service{2, false});
}
std::unique_ptr<mgos::hap::Service> CreateMotionLightService() {
 return std::unique_ptr<mgos::hap::Service>(new mgos::hap::Service{2, true});
}
std::unique_ptr<mgos::hap::Service> CreateHTService(int role, bool temperature) {
 return std::unique_ptr<mgos::hap::Service>(new mgos::hap::Service{role, temperature});
}
std::unique_ptr<mgos::hap::Service> CreateCarPresenceService() {
 return std::unique_ptr<mgos::hap::Service>(new mgos::hap::Service{5, false});
}
std::unique_ptr<mgos::hap::Service> CreateButtonService() {
 return std::unique_ptr<mgos::hap::Service>(new mgos::hap::Service{5, true});
}
std::string state;
int role = -1;
void Finish(const char *) { state = "done"; }
'''
harness += verified + r'''
}
namespace hap {
class GarageDoorOpener {
 public:
 void AddBLUServices(mgos::hap::Accessory *accessory);
 std::vector<std::unique_ptr<mgos::hap::Service>> blu_sensors_;
};
'''
harness += selection + r'''
}}
int main() {
 using namespace shelly;
 for (int mask = 0; mask < 64; mask++) {
  for (int i = 0; i < 6; i++) blu::paired[i] = (mask & (1 << i)) != 0;
  hap::GarageDoorOpener garage;
  mgos::hap::Accessory accessory;
  garage.AddBLUServices(&accessory);
  int counts[6] = {};
  for (auto *service : accessory.services) counts[service->role]++;
  assert(counts[0] == 0 && counts[1] == 0);
  assert(counts[2] == (blu::paired[2] ? 2 : 0));
  assert(counts[3] == (blu::paired[3] ? 2 : 0));
  assert(counts[4] == (blu::paired[4] ? 2 : 0));
  assert(counts[5] == (blu::paired[5] ? 2 : 0));
 }
 for (int selected = 0; selected < 6; selected++) {
  blu::role = selected; blu::state = "waiting_encrypted_beacon";
  blu::PairingVerified((selected + 1) % 6);
  assert(blu::state == "waiting_encrypted_beacon" && mgos::callbacks.empty());
  const int before = restarts;
  blu::PairingVerified(selected);
  assert(blu::state == "done" && restarts == before);
  assert(mgos::callbacks.size() == (selected >= 2 ? 1 : 0));
  for (auto &cb : mgos::callbacks) cb();
  mgos::callbacks.clear();
  assert(restarts == before + (selected >= 2 ? 1 : 0));
  blu::PairingVerified(selected);
  assert(mgos::callbacks.empty());
 }
}
'''
with tempfile.TemporaryDirectory(prefix='shelly-hap-topology-') as temp:
    d = Path(temp)
    (d / 'test.cpp').write_text(harness)
    subprocess.run(['clang++', '-std=c++14', '-Wall', '-Wextra', '-Werror', str(d / 'test.cpp'), '-o', str(d / 'test')], check=True)
    subprocess.run([str(d / 'test')], check=True)
print('PASS: all 64 pairing combinations expose only paired services; verified optional sensors defer one HomeKit refresh')
