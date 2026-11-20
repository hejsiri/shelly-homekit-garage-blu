#!/usr/bin/env python3
"""Exercise the production save/restart path with failed persistence."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/shelly_rpc_service.cpp').read_text()
start = source.index('  if (!st.ok() || !mgos_sys_config_save(')
end = source.index('  SendStatusResp(ri, st);', start)
harness = r'''
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#define LOG(...)
#define STATUS_UNAVAILABLE 14
struct Config {char *name; bool homekit;};
Config mgos_sys_config{};
void mgos_config_free(Config *config) {free(config->name); config->name=nullptr;}
struct Status {bool success; bool ok() const {return success;}};
namespace mgos {Status Errorf(int,const char *) {return {false};}}
bool persistence_ok=true;
int saves=0,restarts=0;
bool mgos_sys_config_save(Config *,bool,void *) {saves++;return persistence_ok;}
void RestartService() {restarts++;}
Status Update(bool valid,bool restart_required) {
 auto free_config=[](Config *config) {mgos_config_free(config);delete config;};
 std::unique_ptr<Config,decltype(free_config)> previous_config(
     new Config{strdup(mgos_sys_config.name),mgos_sys_config.homekit},free_config);
 mgos_config_free(&mgos_sys_config);
 mgos_sys_config={strdup("Changed"),false};
 Status st{valid};
'''+source[start:end]+r'''
 return st;
}
int main() {
 mgos_sys_config={strdup("Original"),true};
 persistence_ok=false;
 assert(!Update(true,true).ok());
 assert(std::string(mgos_sys_config.name)=="Original"&&mgos_sys_config.homekit);
 assert(saves==1&&restarts==0);
 assert(!Update(false,true).ok());
 assert(std::string(mgos_sys_config.name)=="Original"&&mgos_sys_config.homekit);
 assert(saves==1&&restarts==0);
 persistence_ok=true;
 assert(Update(true,true).ok());
 assert(std::string(mgos_sys_config.name)=="Changed"&&!mgos_sys_config.homekit);
 assert(saves==2&&restarts==1);
 assert(Update(true,false).ok());assert(saves==3&&restarts==1);
 mgos_config_free(&mgos_sys_config);
}
'''
with tempfile.TemporaryDirectory(prefix='garage-config-save-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(harness)
    subprocess.run(['clang++', '-std=c++14', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(path / 'test.cpp'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: failed save restores configuration and skips HomeKit restart; successful structural changes restart once')
