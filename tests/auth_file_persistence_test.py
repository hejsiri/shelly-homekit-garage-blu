#!/usr/bin/env python3
"""Fault-test the actual authentication file writer without any device access."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/shelly_rpc_service.cpp').read_text()
start = source.index('static Status WriteAuthFile(')
end = source.index('\nstatic void SetAuthHandler(', start)
writer = source[start:end]
handler = source[end:source.index('\nstatic void GetWifiConfigHandler', end)]
assert handler.index('WriteAuthFile(realm, ha1)') < handler.index('SetAuthFileName(AUTH_FILE_NAME, realm, true)')
assert 'if (!saved.ok())' in handler
assert 'fopen(AUTH_FILE_NAME, "w")' not in handler

harness = r'''
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>
struct Status { bool good; bool ok()const{return good;} static Status OK(){return {true};} };
namespace mgos { static Status Errorf(int,const char*,...){return {false};} }
#define STATUS_UNAVAILABLE 14
#define LOG(...) ((void)0)
#define AUTH_USER "admin"
#define AUTH_FILE_NAME auth_path.c_str()
static std::string auth_path;
enum Fault { None, Open, ShortWrite, Flush, Close, Rename };
static Fault fault=None;
static FILE* candidate=nullptr;
static int old_removals=0;
static FILE* fault_open(const char* path,const char* mode){
 assert(std::string(path)==auth_path+".tmp");assert(mode[0]=='w');
 if(fault==Open){errno=ENOSPC;return nullptr;}
 return candidate=::fopen(path,mode);
}
static size_t fault_write(const void* data,size_t size,size_t count,FILE* fp){
 assert(fp==candidate);
 if(fault==ShortWrite){size_t n=count/2;::fwrite(data,size,n,fp);errno=ENOSPC;return n;}
 return ::fwrite(data,size,count,fp);
}
static int fault_flush(FILE* fp){
 assert(fp==candidate);
 if(fault==Flush){errno=ENOSPC;return EOF;}
 return ::fflush(fp);
}
static int fault_close(FILE* fp){
 assert(fp==candidate);candidate=nullptr;
 int result=::fclose(fp);
 if(fault==Close){errno=ENOSPC;return EOF;}
 return result;
}
static int fault_rename(const char* from,const char* to){
 assert(candidate==nullptr);assert(std::string(from)==auth_path+".tmp");assert(std::string(to)==auth_path);
 if(fault==Rename){errno=ENOSPC;return -1;}
 return ::rename(from,to);
}
static int fault_remove(const char* path){
 if(std::string(path)==auth_path)old_removals++;
 return ::remove(path);
}
#define fopen fault_open
#define fwrite fault_write
#define fflush fault_flush
#define fclose fault_close
#define rename fault_rename
#define remove fault_remove
__WRITER__
#undef fopen
#undef fwrite
#undef fflush
#undef fclose
#undef rename
#undef remove
static bool exists(const std::string& path){struct stat st;return ::stat(path.c_str(),&st)==0;}
static std::string reopen(){std::ifstream fp(auth_path);return {std::istreambuf_iterator<char>(fp),{}};}
int main(int argc,char**argv){
 assert(argc==2);auth_path=argv[1];
 const std::string old_hash(64,'a'),new_hash(64,'b');
 assert(WriteAuthFile("test-device",old_hash.c_str()).ok());
 const std::string old_record="admin:test-device:"+old_hash+"\n";
 assert(reopen()==old_record);
 for(Fault current:{Open,ShortWrite,Flush,Close,Rename}){
  fault=current;
  assert(!WriteAuthFile("test-device",new_hash.c_str()).ok());
  fault=None;
  assert(reopen()==old_record);assert(!exists(auth_path+".tmp"));
  assert(old_removals==0 && candidate==nullptr);
 }
 assert(WriteAuthFile("test-device",new_hash.c_str()).ok());
 assert(reopen()=="admin:test-device:"+new_hash+"\n");
 assert(old_removals==0 && !exists(auth_path+".tmp"));
 auth_path+=".first-save";
 fault=Close;
 assert(!WriteAuthFile("test-device",new_hash.c_str()).ok());
 fault=None;assert(!exists(auth_path)&&!exists(auth_path+".tmp"));
 puts("PASS: actual auth writer preserves credentials on open/short-write/flush/close/rename faults, replaces after complete close, and cleans failed first save");
}
'''.replace('__WRITER__', writer)
with tempfile.TemporaryDirectory(prefix='garage-auth-persistence-') as folder:
    temp = Path(folder)
    (temp / 'check.cpp').write_text(harness)
    subprocess.run(['c++', '-std=c++14', str(temp / 'check.cpp'), '-o', str(temp / 'check')], check=True)
    subprocess.run([str(temp / 'check'), str(temp / 'passwd256')], check=True)
print('PASS: auth handler keeps configuration unchanged when credential write fails')
