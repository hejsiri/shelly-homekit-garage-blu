#!/usr/bin/env python3
"""Exercise the production ToggleState method with recording relay outputs."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/shelly_hap_garage_door_opener.cpp').read_text()
a=s.index('void GarageDoorOpener::ToggleState');b=s.index('\nHAPError ',a)
method=s[a:b]
harness=r'''
#include <cassert>
#include <string>
struct Output { int count=0; std::string source; void Pulse(bool,int,const char *s){count++;source=s;} };
struct Config {bool resume_after_stop=false;int out_mode=0,pulse_time_ms=500;};
class GarageDoorOpener {
public:
enum class State {kOpen,kClosed,kOpening,kClosing,kStopped};
Config config;Config *cfg_=&config; Output close,open;Output *out_close_=&close,*out_open_=&open;
State cur_state_=State::kClosed,tgt_state_=State::kClosed,pre_stopped_state_=State::kStopped;
bool ready=true;
bool SensorsReady(){return ready;}
void SetTgtState(State s,const char*){tgt_state_=s;}
void SetCurState(State s){if(s==State::kStopped)pre_stopped_state_=cur_state_;cur_state_=s;}
void ToggleState(const char*);
};
'''
main=r'''
int main(){
using G=GarageDoorOpener;using S=G::State;
for(bool resume:{false,true})for(bool opening:{false,true})for(int output:{0,1,2}){
 G g;g.config.resume_after_stop=resume;g.config.out_mode=output;
 g.cur_state_=opening?S::kOpening:S::kClosing;g.tgt_state_=opening?S::kOpen:S::kClosed;
 g.ToggleState("test");assert(g.cur_state_==S::kStopped);assert(g.close.count+g.open.count==1);
 g.ToggleState("test");bool next=resume?opening:!opening;
 assert(g.cur_state_==(next?S::kOpening:S::kClosing));assert(g.tgt_state_==(next?S::kOpen:S::kClosed));
 assert(g.close.count+g.open.count==2);
 if(output==1)assert((next?g.open.source:g.close.source)==(next?"GDO:open":"GDO:close"));
}
G g;g.ready=false;g.ToggleState("test");assert(g.close.count+g.open.count==0);
g.ready=true;g.ToggleState("test");assert(g.cur_state_==S::kOpening);
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.cpp').write_text(harness+method+main)
 subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: stop both directions, reverse/continue modes, relay routing and blocked controls')
