#!/usr/bin/env python3
"""Exercise the production one-time HomeKit schema refresh without resetting pairings."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/shelly_main.cpp').read_text()
start=source.index('static bool MigrateBLUHAPLayout()');end=source.index('\n#endif',start)
function=source[start:end]
harness=r'''
#include <cassert>
#define LOG(...)
const int kHAPError_None=0;
int revision=0,increments=0,result=0,s_kvs=0;
int mgos_sys_config_get_blu_hap_layout_version(){return revision;}
void mgos_sys_config_set_blu_hap_layout_version(int v){revision=v;}
int HAPAccessoryServerIncrementCN(int*){increments++;return result;}
'''+function+r'''
int main(){
 assert(MigrateBLUHAPLayout()&&revision==3&&increments==1);
 assert(!MigrateBLUHAPLayout()&&revision==3&&increments==1);
 revision=1;assert(MigrateBLUHAPLayout()&&revision==3&&increments==2);
 revision=2;assert(MigrateBLUHAPLayout()&&revision==3&&increments==3);
 revision=0;result=1;assert(!MigrateBLUHAPLayout()&&revision==0&&increments==4);
 result=0;assert(MigrateBLUHAPLayout()&&revision==3&&increments==5);
 revision=4;assert(!MigrateBLUHAPLayout()&&revision==4&&increments==5);
}
'''
with tempfile.TemporaryDirectory(prefix='shelly-hap-migration-') as temp:
 p=Path(temp);(p/'test.cpp').write_text(harness)
 subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
assert 'if (MigrateBLUHAPLayout()) changed = true;' in source
print('PASS: one-time HomeKit config refresh, retry on storage failure, no downgrade and no pairing reset')
