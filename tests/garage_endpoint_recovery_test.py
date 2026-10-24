#!/usr/bin/env python3
"""Run the production state machine after timeouts and delayed endpoint reports."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/shelly_hap_garage_door_opener.cpp').read_text()
def method(signature, next_signature):
    start = source.index(signature)
    return source[start:source.index(next_signature, start)]

harness = r'''
#include <cassert>
#include <cstdint>
#include <initializer_list>
#define LOG(...)
int64_t now_us = 0;
int64_t mgos_uptime_micros() { return now_us; }
struct Event { int count=0; void RaiseEvent(){count++;} };
struct Config { int move_time_ms=10000,begin_move_time_ms=1000; };
class GarageDoorOpener {
public:
 enum class State {kOpen,kClosed,kOpening,kClosing,kStopped};
 Config config; Config *cfg_=&config;
 Event current,target,obstruction;
 Event *cur_state_char_=&current,*tgt_state_char_=&target,*obst_char_=&obstruction;
 State cur_state_=State::kStopped,tgt_state_=State::kClosed,pre_stopped_state_=State::kStopped;
 int64_t begin_=0; bool ready=true,sensors_ready_=true,obstruction_detected_=false,movement_timed_out_=false;
 int closed=0,open=0;
 bool SensorsReady(){return ready;}
 void GetInputsState(int *c,int *o)const{*c=closed;*o=open;}
 void SetCurState(State); void SetTgtState(State,const char*); void RunOnce();
};
'''
harness += method('void GarageDoorOpener::SetCurState(', '\nvoid GarageDoorOpener::ToggleState(')
harness += method('void GarageDoorOpener::SetTgtState(', '\nvoid GarageDoorOpener::RunOnce(')
harness += method('void GarageDoorOpener::RunOnce(', '\n#ifdef SHELLY_GARAGE_BLE\nvoid GarageDoorOpener::AddBLUServices(')
harness += r'''
int main(){
 using G=GarageDoorOpener;using S=G::State;
 for(auto direction:{S::kOpening,S::kClosing})for(bool closing_endpoint:{false,true}){
  G g;now_us=0;g.SetCurState(direction);g.tgt_state_=direction==S::kOpening?S::kOpen:S::kClosed;
  now_us=11000000;g.RunOnce();
  assert(g.cur_state_==S::kStopped&&g.movement_timed_out_&&g.obstruction_detected_);
  const int fault_events=g.obstruction.count;
  g.RunOnce();assert(g.obstruction_detected_); // No endpoint: preserve the fault.
  g.ready=false;g.closed=1;g.RunOnce();assert(g.obstruction_detected_); // Stale data cannot clear it.
  g.ready=true;g.closed=g.open=1;g.RunOnce();assert(g.obstruction_detected_); // Neither can a conflict.
  g.closed=closing_endpoint;g.open=!closing_endpoint;
  g.RunOnce();
  const S endpoint=closing_endpoint?S::kClosed:S::kOpen;
  assert(g.cur_state_==endpoint&&g.tgt_state_==endpoint);
  assert(!g.movement_timed_out_&&!g.obstruction_detected_);
  assert(g.obstruction.count==fault_events+1);
  const int notifications=g.obstruction.count;
  g.RunOnce();assert(g.cur_state_==endpoint&&g.obstruction.count==notifications);
 }
}
'''
with tempfile.TemporaryDirectory() as temp:
    p = Path(temp)
    (p/'test.cpp').write_text(harness)
    subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror','-Wno-unused-parameter',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
print('PASS: either confirmed endpoint clears timeout and HomeKit obstruction; stale/conflicting inputs do not')
