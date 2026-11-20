#!/usr/bin/env python3
"""Check OTA and startup reclamation against real files, preserving user data."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/shelly_main.cpp').read_text()
a = source.index('extern "C" bool mgos_ota_merge_fs_should_copy_file(')
b = source.index('\nstatic void HTTPHandler(', a)
merge = source[a:b]
harness = r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include "shelly_obsolete_garage_assets.hpp"
using namespace shelly;
'''+merge+r'''
void Put(const std::string &name) { std::ofstream file(name); file << "retained"; }
bool Exists(const std::string &name) { struct stat s; return stat(name.c_str(), &s) == 0; }
int main(int argc, char **argv) {
  assert(argc == 2); assert(chdir(argv[1]) == 0);
  const char *retained[] = {"conf0.json", "conf9.json", "kvs.json", "kvs.json.tmp",
      "rpc.acl", "auth.json", "log_0.txt", "garage-v16-day.webp",
      "garage-v16-night.webp", "garage-v16-door-day.webp", "garage-v16-door-night.webp",
      "garage-v16-interior-day.webp", "garage-v16-interior-night.webp",
      "garage-tesla-s-day.webp", "garage-tesla-s-night.webp",
      "garage-icon-open.svg", "garage-icon-closed.svg", "my-photo.webp"};
  for (const char *name : retained) Put(name);
  for (const char *name : kObsoleteGarageAssets) {
    Put(name);
    assert(!mgos_ota_merge_fs_should_copy_file("/old", "/missing", name));
  }
  assert(!IsObsoleteGarageAsset(nullptr));
  assert(!IsObsoleteGarageAsset("/garage-car.webp"));
  assert(!IsObsoleteGarageAsset("garage-car.webp.bak"));
  assert(RemoveObsoleteGarageAssets(::remove) == 11);
  assert(RemoveObsoleteGarageAssets(::remove) == 0);  // Reboot is idempotent.
  for (const char *name : kObsoleteGarageAssets) assert(!Exists(name));
  for (const char *name : retained) {
    assert(Exists(name)); assert(!IsObsoleteGarageAsset(name));
    std::ifstream file(name); std::string value; file >> value; assert(value == "retained");
    assert(mgos_ota_merge_fs_should_copy_file("/old", "/missing", name));
  }
  // A deletion failure is retained for a later boot; other files still clear.
  for (const char *name : kObsoleteGarageAssets) Put(name);
  assert(RemoveObsoleteGarageAssets([](const char *name) {
    return strcmp(name, "garage-car.webp") == 0 ? -1 : ::remove(name);
  }) == 10);
  assert(Exists("garage-car.webp"));
  assert(RemoveObsoleteGarageAssets(::remove) == 1);
  for (const char *name : retained) assert(Exists(name));
}
'''
with tempfile.TemporaryDirectory(prefix='shelly-fs-cleanup-') as temp:
    p = Path(temp)
    (p / 'test.cpp').write_text(harness)
    (p / 'files').mkdir()
    subprocess.run(['clang++', '-std=c++14', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I', str(root / 'src'),
                    str(p / 'test.cpp'), '-o', str(p / 'test')], check=True)
    subprocess.run([str(p / 'test'), str(p / 'files')], check=True)

init = source[source.index('void InitApp()'):]
assert init.index('RemoveObsoleteGarageAssets(::remove)') < init.index('InitWifiConfigManager()')
assert init.index('RemoveObsoleteGarageAssets(::remove)') < init.index('CheckRebootCounter()')
assert init.index('RemoveObsoleteGarageAssets(::remove)') < init.index('HAPPlatformKeyValueStoreCreate(')
print('PASS: exact obsolete assets removed before persistence; OTA exclusion; user data/current assets retained; retry and reboot idempotence')
