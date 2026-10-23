// Exercise the real pairing UI callbacks with asynchronous RPC responses.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const script = fs.readFileSync('fs_src/script.js', 'utf8');
const elements = {};
for (const id of ['blu_pairing','blu_pair_status','blu_pair_candidate','blu_pair_current_container','blu_pair_confirm','blu_pair_pin','blu_pair_current_pin']) {
  elements[id] = {value:'', style:{display:''}, replaceChildren(...children) { this.children=children; }};
}
let status = {state:'scanning', candidates:[{mac:'AA:BB:CC:DD:EE:03',rssi:-50,encrypted:true}]};
let rejectPair = false;
const ctx = {el:(_,id)=>elements[id], updateInnerText:(e,t)=>{e.textContent=t;},
 document:{createElement:()=>({})}, alert:()=>{throw Error('Unexpected validation alert');},
 callDevice:(method)=>method==='Shelly.BLU.Status'?Promise.resolve(status):rejectPair?Promise.reject(Error('Rejected')):Promise.resolve({})};
vm.createContext(ctx);
vm.runInContext(script.slice(script.indexOf('function bluPairStatus('),script.indexOf('function gdoSetConfig(')),ctx);
const c={bluPairRole:0};
const tick=()=>new Promise(resolve=>setImmediate(resolve));
(async()=>{
 for(const state of ['scanning','error','done']) {
  status={...status,state,error:state==='error'?'Timed out':''};
  elements.blu_pair_pin.value='135790';elements.blu_pair_current_pin.value='975310';
  for(let i=0;i<3;i++) {ctx.bluPairRefresh(c);await tick();}
  assert.equal(elements.blu_pair_pin.value,'135790',state);
  assert.equal(elements.blu_pair_current_pin.value,'975310',state);
 }
 rejectPair=true;ctx.bluPairConfirm(c);await tick();
 assert.equal(elements.blu_pair_pin.value,'135790');
 assert.equal(elements.blu_pair_current_pin.value,'975310');
 rejectPair=false;ctx.bluPairConfirm(c);await tick();
 assert.equal(elements.blu_pair_pin.value,'');assert.equal(elements.blu_pair_current_pin.value,'');
 console.log('PASS: status polling and rejected pairing preserve PINs; accepted pairing clears both');
})().catch(e=>{console.error(e);process.exitCode=1;});
