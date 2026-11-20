#!/usr/bin/env python3
"""Run actual prepared HomeKit KVS and Frozen code against persistence faults."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ.get('SHELLY_HOMEKIT_ADK_SOURCE', str(Path.home() / '.mos/work/shelly-garage-blu/deps/homekit-adk')))
frozen = sdk.parent / 'modules/mongoose-os/src/frozen'
spec = importlib.util.spec_from_file_location('kvs_patch', root / 'tools/patch-homekit-kvs.py')
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)
source = (sdk / 'src/PAL/HAPPlatformKeyValueStore.cpp').read_text()
patched = patch.patch_source(source)
assert patch.patch_source(patched) == patched
try:
    patch.patch_source(source.replace('HAPError KVStore::Save() const {', 'HAPError KVStore::Save() const {\n// upstream changed'))
except ValueError:
    pass
else:
    raise AssertionError('Unreviewed upstream change was accepted')

headers = {
    'HAPPlatformKeyValueStore.h': '''#pragma once
#include <stdint.h>
#include <stddef.h>
#define _Nullable
typedef enum { kHAPError_None=0, kHAPError_Unknown=1, kHAPError_OutOfResources=2 } HAPError;
typedef uint8_t HAPPlatformKeyValueStoreDomain;
typedef uint8_t HAPPlatformKeyValueStoreKey;
typedef struct { void* ctx; } HAPPlatformKeyValueStore;
typedef HAPPlatformKeyValueStore* HAPPlatformKeyValueStoreRef;
typedef HAPError (*HAPPlatformKeyValueStoreEnumerateCallback)(void*,HAPPlatformKeyValueStoreRef,HAPPlatformKeyValueStoreDomain,HAPPlatformKeyValueStoreKey,bool*);
''',
    'HAPPlatformKeyValueStore+Init.h': '''#pragma once
typedef struct { const char* fileName; } HAPPlatformKeyValueStoreOptions;
''',
    'mgos.hpp': '''#pragma once
#include "frozen.h"
#include <algorithm>
#include <cstdlib>
#define LOG(...) ((void)0)
namespace mgos { struct ScopedCPtr { void* p; explicit ScopedCPtr(void* value):p(value){} ~ScopedCPtr(){free(p);} }; }
static char* cs_read_file(const char* path,size_t* size){
 FILE* fp=::fopen(path,"rb");if(!fp)return nullptr;
 fseek(fp,0,SEEK_END);long n=ftell(fp);rewind(fp);
 char* p=static_cast<char*>(malloc(n+1));*size=fread(p,1,n,fp);p[*size]=0;::fclose(fp);return p;
}
''',
}
harness = r'''
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include "mgos.hpp"
enum Fault { None, Open, ShortWrite, Flush, Close, Rename };
static Fault fault=None;
static FILE* candidate=nullptr;
static std::string main_path;
static int main_removals=0;
static FILE* checked_open(const char* path,const char* mode){
 if(fault==Open && std::string(path)==main_path+".tmp"){errno=ENOSPC;return nullptr;}
 FILE* fp=::fopen(path,mode);
 if(std::string(path)==main_path+".tmp" && mode[0]=='w')candidate=fp;
 return fp;
}
static size_t checked_write(const void* data,size_t size,size_t count,FILE* fp){
 if(fp==candidate && fault==ShortWrite){
  // A positive short write without ferror exercises the checked printer itself.
  size_t actual=count>1?count-1:0;
  if(actual)::fwrite(data,size,actual,fp);
  errno=ENOSPC;return actual;
 }
 return ::fwrite(data,size,count,fp);
}
static int checked_flush(FILE* fp){
 if(fp==candidate && fault==Flush){errno=ENOSPC;return EOF;}
 return ::fflush(fp);
}
static int checked_close(FILE* fp){
 bool fail=fp==candidate && fault==Close;
 if(fp==candidate)candidate=nullptr;
 int result=::fclose(fp);
 if(fail){errno=ENOSPC;return EOF;}
 return result;
}
static int checked_rename(const char* from,const char* to){
 if(fault==Rename){errno=ENOSPC;return -1;}
 return ::rename(from,to);
}
static int checked_remove(const char* path){
 if(std::string(path)==main_path)main_removals++;
 return ::remove(path);
}
#define fopen checked_open
#define fwrite checked_write
#define fflush checked_flush
#define fclose checked_close
#define rename checked_rename
#define remove checked_remove
#include "KVS.cpp"
#undef fopen
#undef fwrite
#undef fflush
#undef fclose
#undef rename
#undef remove
static std::string read_disk(){std::ifstream f(main_path);return {std::istreambuf_iterator<char>(f),{}};}
static bool exists(const std::string& path){struct stat st;return ::stat(path.c_str(),&st)==0;}
static void check_value(const std::string& wanted){
 HAPPlatformKeyValueStore store={};HAPPlatformKeyValueStoreOptions opts={main_path.c_str()};
 HAPPlatformKeyValueStoreCreate(&store,&opts);
 char out[100]={};size_t n=0;bool found=false;
 assert(HAPPlatformKeyValueStoreGet(&store,4,0,out,sizeof(out),&n,&found)==kHAPError_None);
 assert(found && std::string(out,n)==wanted);
 HAPPlatformKeyValueStoreRelease(&store);
}
int main(int argc,char**argv){
 assert(argc==2);main_path=argv[1];
 const std::string old(70,'A'),updated(70,'B');
 HAPPlatformKeyValueStore store={};HAPPlatformKeyValueStoreOptions opts={main_path.c_str()};
 HAPPlatformKeyValueStoreCreate(&store,&opts);
 assert(HAPPlatformKeyValueStoreSet(&store,4,0,old.data(),old.size())==kHAPError_None);
 HAPPlatformKeyValueStoreRelease(&store);
 const auto original=read_disk();assert(!original.empty());check_value(old);
 for(Fault current:{Open,ShortWrite,Flush,Close,Rename}){
  fault=None;HAPPlatformKeyValueStoreCreate(&store,&opts);fault=current;
  assert(HAPPlatformKeyValueStoreSet(&store,4,0,updated.data(),updated.size())!=kHAPError_None);
  fault=None;
  assert(read_disk()==original);
  assert(!exists(main_path+".tmp"));assert(main_removals==0);
  HAPPlatformKeyValueStoreRelease(&store);check_value(old);
 }
 HAPPlatformKeyValueStoreCreate(&store,&opts);
 assert(HAPPlatformKeyValueStoreSet(&store,4,0,updated.data(),updated.size())==kHAPError_None);
 HAPPlatformKeyValueStoreRelease(&store);check_value(updated);
 assert(main_removals==0 && !exists(main_path+".tmp"));
 // Failed first pairing must not leave a partial .tmp for Load's fallback.
 main_path += ".new";opts.fileName=main_path.c_str();
 HAPPlatformKeyValueStoreCreate(&store,&opts);fault=Close;
 assert(HAPPlatformKeyValueStoreSet(&store,4,0,updated.data(),updated.size())!=kHAPError_None);
 fault=None;HAPPlatformKeyValueStoreRelease(&store);
 assert(!exists(main_path) && !exists(main_path+".tmp"));
 puts("PASS: actual KVS/Frozen reopen, open/positive-short-write/flush/close/rename faults retain old keys, successful replacement, failed first-save cleanup");
}
'''
with tempfile.TemporaryDirectory(prefix='garage-kvs-persistence-') as folder:
    temp = Path(folder)
    for name, content in headers.items():
        (temp / name).write_text(content)
    (temp / 'KVS.cpp').write_text(patched)
    (temp / 'check.cpp').write_text(harness)
    subprocess.run(['cc', '-c', '-I' + str(frozen), str(frozen / 'frozen.c'), '-o', str(temp / 'frozen.o')], check=True)
    subprocess.run(['c++', '-std=c++14', '-I' + str(temp), '-I' + str(frozen), str(temp / 'check.cpp'), str(temp / 'frozen.o'), '-o', str(temp / 'check')], check=True)
    subprocess.run([str(temp / 'check'), str(temp / 'kvs.json')], check=True)
print('PASS: exact upstream guard and idempotent KVS build patch')
