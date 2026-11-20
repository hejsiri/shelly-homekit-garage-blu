#!/usr/bin/env python3
"""Run the production Wi-Fi trial/boot/rollback code against a mock radio."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/wifi_config/shelly_wifi_config_impl.cpp').read_text()


def block_at(start):
    begin = source.index('{', start)
    depth = 0
    for end in range(begin, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if depth == 0:
            return source[begin:end + 1]
    raise AssertionError('Unclosed production block')


set_body = block_at(source.index('Status WifiConfigManager::SetConfig('))
boot_start = source.index('  cur_.sta_ps_mode = mgos_sys_config_get_wifi_sta_ps_mode();')
boot_body = source[boot_start:source.index('  mgos_event_add_group_handler(', boot_start)]
rollback_start = source.index('if (act_ == &new_)', source.index('connect_failed_ = true;'))
rollback_body = block_at(rollback_start)

harness = r'''
#include <cassert>
#include <cstdlib>
#include <string>
#include "shelly_wifi_policy.hpp"
#define LOG(...) ((void) 0)
#define LL_WARN 1
#define LL_ERROR 2
constexpr int STATUS_INVALID_ARGUMENT = 1;
struct Status { bool success = true; bool ok() const { return success; }
  static Status OK() { return {}; } };
namespace mgos {
template<class... Args> Status Errorf(Args...) { return {false}; }
template<class... Args> Status Annotatef(Status status, Args...) { return status; }
}
struct Network {
  int value = 0;
  bool operator==(const Network &other) const { return value == other.value; }
};
struct WifiConfig {
  Network sta, sta1, ap; int sta_ps_mode = 1;
  std::string ToJSON() const { return "{}"; }
};
int sys_mode = 1, mode_writes = 0, saves = 0, radio_mode = -1;
int mgos_sys_config;
int mgos_sys_config_get_wifi_sta_ps_mode() { return sys_mode; }
void mgos_sys_config_set_wifi_sta_ps_mode(int value) { sys_mode = value; ++mode_writes; }
bool mgos_sys_config_save(int *, bool, char **) { ++saves; return true; }
using esp_err_t = int;
using wifi_ps_type_t = int;
constexpr int ESP_OK = 0;
int esp_wifi_set_ps(int value) { radio_mode = value; return ESP_OK; }
namespace shelly {
class WifiConfigManager {
 public:
  enum class State { kIdle, kDisconnect };
  WifiConfig cur_, new_;
  WifiConfig *act_ = &cur_;
  bool ap_config_changed_ = false, connect_failed_ = false;
  int disconnects = 0;
  Status ValidateAPConfig(Network) { return Status::OK(); }
  Status ValidateSTAConfig(Network) { return Status::OK(); }
  void SetState(State) { ++disconnects; }
  Status SetConfig(const WifiConfig &cfg) SET_BODY
  void Boot() { BOOT_BODY }
  void Rollback() ROLLBACK_BODY
};
}
int main() {
  using namespace shelly;
  for (int mode = -5; mode <= 5; ++mode) {
    assert(WifiPowerSaveModeAllowed(mode) == (mode >= kWifiMinPowerSaveMode && mode <= 2));
    const int recovered = WifiBootPowerSaveMode(mode);
    assert(WifiPowerSaveModeAllowed(recovered));
    if (WifiPowerSaveModeAllowed(mode)) assert(recovered == mode);
  }
  WifiConfigManager manager;
  WifiConfig attempt = manager.cur_;
  attempt.sta_ps_mode = 0;
  attempt.sta.value = 42;  // Credentials must not change when mode is rejected.
  Status status = manager.SetConfig(attempt);
  if (kWifiMinPowerSaveMode > 0) {
    assert(!status.ok());
    assert(mode_writes == 0 && manager.disconnects == 0);
    assert(manager.act_ == &manager.cur_ && manager.cur_.sta.value == 0);
  } else {
    assert(status.ok() && sys_mode == 0 && manager.disconnects == 1);
  }
  WifiConfigManager trial;
  sys_mode = 1;
  attempt = trial.cur_;
  attempt.sta_ps_mode = 2;
  attempt.sta.value = 99;
  assert(trial.SetConfig(attempt).ok());
  assert(sys_mode == 2 && trial.act_ == &trial.new_ && trial.disconnects == 1);
  trial.Rollback();
  assert(sys_mode == 1 && trial.act_ == &trial.cur_ && trial.cur_.sta.value == 0);
  for (int stored : {0, 1, 2}) {
    WifiConfigManager boot;
    sys_mode = stored; saves = 0; radio_mode = -1;
    boot.Boot();
    assert(boot.cur_.sta_ps_mode == WifiBootPowerSaveMode(stored));
    assert(sys_mode == boot.cur_.sta_ps_mode);
    const bool migrated = stored < kWifiMinPowerSaveMode;
    assert(saves == (migrated ? 1 : 0));
    if (migrated) assert(radio_mode == kWifiMinPowerSaveMode);
  }
}
'''.replace('SET_BODY', set_body).replace('BOOT_BODY', boot_body).replace('ROLLBACK_BODY', rollback_body)

with tempfile.TemporaryDirectory(prefix='wifi-power-policy-') as folder:
    path = Path(folder)
    test = path / 'test.cpp'
    test.write_text(harness)
    for minimum in (0, 1):
        binary = path / f'test-{minimum}'
        subprocess.run(['clang++', '-std=c++14', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', f'-DSHELLY_WIFI_MIN_PS_MODE={minimum}',
                        '-I' + str(root / 'src'), str(test), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
print('PASS: production guard rejects mode 0 before side effects; boot migration persists/applies mode 1; rollback restores radio mode; other models keep mode 0')
