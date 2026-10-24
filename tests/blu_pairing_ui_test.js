// Exercise the production pairing wizard and delayed RPC replies.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const script = fs.readFileSync('fs_src/script.js', 'utf8');
const source = script.slice(script.indexOf('function bluPairStatus('), script.indexOf('function gdoSetConfig('));
const tick = () => new Promise(resolve => setImmediate(resolve));
const deferred = () => {
  let resolve, reject;
  const promise = new Promise((yes, no) => { resolve = yes; reject = no; });
  return {promise, resolve, reject};
};
const sensor = (model = 2, encrypted = false, mac = 'AA:BB:CC:DD:EE:03') => ({mac, model, encrypted, rssi:-50});
function fixture(candidates = [sensor()]) {
  const elements = {};
  for (const id of ['pair_xiaomi_container','pair_bindkey','pair_xiaomi_mac','closed_type','open_type','indoor_type','outdoor_type','pairing','pair_title','pair_step','pair_help','pair_selected','pair_status','pair_candidate_container',
    'pair_candidate','pair_new_container','pair_current_container','pair_current_summary','pair_confirm','pair_cancel','pair_pin','pair_current_pin']) {
    elements['blu_' + id] = {open:false, value:'', disabled:false, style:{display:''},
      showModal(){ this.open=true; }, close(){ this.open=false; }, focus(){ this.focused=true; },
      replaceChildren(...children){ this.children=children; this.value=children[0]?.value || ''; }};
  }
  const calls=[], handlers={};
  let status={state:'scanning', candidates, error:''};
  const ctx={lastInfo:{auth_en:true}, el:(_,id)=>elements[id], updateInnerText:(e,t)=>{ e.textContent=t; },
    document:{createElement:()=>({})}, alert:()=>{ throw Error('Unexpected alert'); },
    callDevice(method,params){
      calls.push({method, params});
      if (handlers[method]) return handlers[method](params);
      if (method==='Shelly.BLU.Status') return Promise.resolve({...status});
      if (method==='Shelly.BLU.Pair') status={...status,state:'connecting'};
      if (method==='Shelly.BLU.SetPIN') status={...status,state:'setting_pin'};
      if (method==='Shelly.BLU.Resume') status={...status,state:'waiting_pairing_mode'};
      if (method==='Shelly.BLU.Cancel') status={...status,state:'error',error:'Pairing cancelled'};
      return Promise.resolve({});
    }};
  vm.createContext(ctx); vm.runInContext(source,ctx);
  const c={};
  return {ctx,c,e:elements,calls,handlers,setStatus:(value)=>{status=value;},
    count:(method)=>calls.filter(call=>call.method==='Shelly.BLU.'+method).length,
    start(role=0){ctx.bluPairStart(c,role);},
    async refresh(value){status=value;ctx.bluPairRefresh(c);await tick();},
    hidden(id){assert.equal(elements['blu_'+id].style.display,'none',id+' must be hidden');},
    visible(id){assert.notEqual(elements['blu_'+id].style.display,'none',id+' must be visible');},
  };
}
(async()=>{
  // Opening the modal starts discovery and pairing automatically, without a PIN.
  let f=fixture();f.start();
  assert.equal(f.e.blu_pair_current_container.open,false);
  f.hidden('pair_new_container');
  f.visible('pair_status');
  f.ctx.bluPairRefresh(f.c);assert.equal(f.calls.length,0);
  f.ctx.bluPairConfirm(f.c);f.ctx.bluPairConfirm(f.c);
  f.hidden('pair_new_container');f.hidden('pair_current_container');
  await tick();assert.equal(f.count('Discover'),1);assert.equal(f.count('Pair'),1);
  assert.equal(f.calls.find(call=>call.method==='Shelly.BLU.Pair').params.pin,undefined);
  assert.equal(f.c.bluPairCredentials,null);assert.equal(f.e.blu_pair_pin.value,'');
  f.visible('pair_selected');f.hidden('pair_candidate_container');f.hidden('pair_confirm');
  for(const state of ['verifying_existing_key','connecting','securing_link','discovering_services','setting_key','setting_pin','waiting_key_ready','reading_key','waiting_security_retry','waiting_encrypted_beacon']) {
    await f.refresh({state,candidates:[sensor(2,true)],error:''});
    f.hidden('pair_new_container');f.hidden('pair_current_container');f.hidden('pair_candidate_container');f.hidden('pair_confirm');
    assert.equal(f.count('Pair'),1);
    if(state==='waiting_security_retry') {
      assert.match(f.e.blu_pair_status.textContent,/Waiting to reconnect/);
      assert.match(f.e.blu_pair_status.className,/blu-pair-waiting/);
      assert.match(f.e.blu_pair_help.textContent,/automatically/);
    }
  }
  // A physical wake-up resumes on the backend; there is no extra panel action.
  await f.refresh({state:'waiting_user_pairing',candidates:[],error:''});
  f.hidden('pair_new_container');f.hidden('pair_current_container');f.hidden('pair_confirm');
  assert.match(f.e.blu_pair_help.textContent,/resumes automatically/);
  f.ctx.bluPairConfirm(f.c);f.ctx.bluPairConfirm(f.c);await tick();
  assert.equal(f.count('Resume'),0);
  await f.refresh({state:'connecting',candidates:[],error:''});f.hidden('pair_confirm');
  await f.refresh({state:'done',candidates:[],error:''});f.hidden('pair_new_container');
  await f.ctx.bluPairClose(f.c);assert.equal(f.e.blu_pairing.open,false);assert.equal(f.count('Cancel'),0);

  // A failed re-pair is distinct from recent, authenticated sensor readings.
  f=fixture();f.start(1);f.ctx.bluPairConfirm(f.c);await tick();
  const mac=sensor().mac;
  f.c.bluPairTelemetry={open_mac:mac.toLowerCase(),stale_after_s:180,
    open:{secure:true,seen:true,age_s:6}};
  await f.refresh({state:'error',error:'Device disconnected during securing_link',candidates:[]});
  assert.match(f.e.blu_pair_help.textContent,/Recent encrypted readings/);
  assert.equal(f.c.bluPairState,'error');assert.match(f.e.blu_pair_status.className,/error/);
  f.hidden('pair_new_container');f.hidden('pair_current_container');f.hidden('pair_confirm');
  for(const patch of [{secure:false},{seen:false},{age_s:16},{age_s:-1},{age_s:NaN}]) {
    f.c.bluPairTelemetry.open={secure:true,seen:true,age_s:6,...patch};
    f.ctx.bluPairRender(f.c);assert.doesNotMatch(f.e.blu_pair_help.textContent,/Recent encrypted readings/);
  }
  f.c.bluPairTelemetry.open={secure:true,seen:true,age_s:2};
  f.c.bluPairTelemetry.open_mac='11:22:33:44:55:66';
  f.ctx.bluPairRender(f.c);assert.doesNotMatch(f.e.blu_pair_help.textContent,/Recent encrypted readings/);
  f.c.bluPairTelemetry.open_mac=mac;
  f.ctx.bluPairRender(f.c);assert.match(f.e.blu_pair_help.textContent,/Recent encrypted readings/);

  // A delayed scanning reply after accepted pairing cannot ask for credentials again.
  f=fixture();f.start();f.ctx.bluPairConfirm(f.c);await tick();
  await f.refresh({state:'scanning',candidates:[sensor(2,true)],error:''});
  assert.equal(f.c.bluPairState,'connecting');f.hidden('pair_new_container');f.hidden('pair_current_container');
  assert.equal(f.count('Pair'),1);

  // Legacy BLE authentication still requires its current PIN, entered once.
  f=fixture([sensor(2,true)]);f.start();f.ctx.bluPairConfirm(f.c);await tick();
  assert.equal(f.count('Pair'),0);f.hidden('pair_new_container');f.visible('pair_current_container');
  assert.match(f.e.blu_pair_help.textContent,/existing PIN/);
  f.e.blu_pair_current_pin.value='123';f.ctx.bluPairConfirm(f.c);await tick();
  assert.equal(f.count('Pair'),0);assert.match(f.e.blu_pair_status.textContent,/six-digit/);
  f.ctx.bluPairRefresh(f.c);await tick();assert.match(f.e.blu_pair_status.textContent,/six-digit/);
  f.e.blu_pair_current_pin.oninput();assert.doesNotMatch(f.e.blu_pair_status.textContent,/six-digit/);
  f.e.blu_pair_current_pin.value='975310';f.ctx.bluPairConfirm(f.c);await tick();
  const submitted=f.calls.find(call=>call.method==='Shelly.BLU.Pair').params;
  assert.equal(submitted.pin,undefined);assert.equal(submitted.current_pin,'975310');
  f.hidden('pair_current_container');assert.equal(f.c.bluPairCredentials,null);
  // Encryption activation asks for a PIN only when the backend requires one.
  f=fixture();f.start();await tick();
  await f.refresh({state:'waiting_encryption_pin',candidates:[],error:''});
  f.visible('pair_new_container');f.hidden('pair_current_container');
  assert.equal(f.e.blu_pair_confirm.textContent,'Enable encryption');
  f.ctx.bluPairConfirm(f.c);await tick();assert.equal(f.count('SetPIN'),0);
  f.e.blu_pair_pin.value='135790';f.ctx.bluPairConfirm(f.c);f.ctx.bluPairConfirm(f.c);await tick();
  assert.equal(f.count('SetPIN'),1);assert.equal(f.count('Pair'),1);
  assert.equal(f.calls.find(call=>call.method==='Shelly.BLU.SetPIN').params.pin,'135790');
  f.hidden('pair_new_container');assert.equal(f.e.blu_pair_pin.value,'');

  // All six roles filter the discovered model before selecting a unique device automatically.
  for(const [role,model] of [[0,2],[1,2],[2,5],[3,3],[4,3],[5,1]]) {
    f=fixture([sensor(model),sensor(model===2?5:2,false,'AA:BB:CC:DD:EE:08')]);f.start(role);
    f.ctx.bluPairConfirm(f.c);await tick();
    assert.equal(f.count('Pair'),1);assert.equal(f.calls.find(call=>call.method==='Shelly.BLU.Pair').params.mac,'AA:BB:CC:DD:EE:03');
  }
  // Stock Xiaomi is discovered by its selected model, with no PIN or wake instruction.
  f=fixture([sensor(426),sensor(3,false,'AA:BB:CC:DD:EE:08')]);
  f.e.blu_indoor_type.value='xiaomi';f.start(3);
  assert.match(f.e.blu_pair_help.textContent,/no pairing mode or PIN/);
  assert.doesNotMatch(f.e.blu_pair_help.textContent,/10 seconds/);
  await tick();assert.equal(f.count('Pair'),1);
  assert.equal(f.calls.find(call=>call.method==='Shelly.BLU.Discover').params.model,426);
  await f.refresh({state:'waiting_beacon',candidates:[],error:''});
  assert.match(f.e.blu_pair_status.textContent,/temperature and humidity/);
  assert.match(f.e.blu_pair_status.className,/waiting/);
  f.hidden('pair_current_container');f.hidden('pair_new_container');
  f=fixture([sensor(426)]);f.e.blu_outdoor_type.value='xiaomi';f.start(4);await tick();
  assert.equal(f.count('Pair'),1);
  f=fixture([sensor(426)]);f.start(3);await tick();assert.equal(f.count('Pair'),0);
  // Stock LYWSD03MMC: one bindkey field, optional manual MAC, no legacy PIN prompt.
  f=fixture([]);f.e.blu_indoor_type.value='xiaomi03';f.start(3);await tick();
  f.visible('pair_xiaomi_container');f.hidden('pair_current_container');f.hidden('pair_new_container');
  assert.equal(f.calls.find(call=>call.method==='Shelly.BLU.Discover').params.model,1371);
  assert.equal(f.e.blu_pair_confirm.disabled,true);
  f.e.blu_pair_bindkey.value='123456';f.e.blu_pair_bindkey.oninput();
  assert.equal(f.e.blu_pair_confirm.disabled,true);
  f.e.blu_pair_bindkey.value='e9ea895fac7cca6d30532432a516f3a8';
  f.e.blu_pair_xiaomi_mac.value='A4:C1:38:02:83:F4';f.e.blu_pair_xiaomi_mac.oninput();
  assert.equal(f.e.blu_pair_confirm.disabled,false);
  f.ctx.bluPairConfirm(f.c);await tick();
  let stockCall=f.calls.find(call=>call.method==='Shelly.BLU.Pair');
  assert.equal(stockCall.params.mac,'A4:C1:38:02:83:F4');assert.equal(stockCall.params.bindkey.length,32);
  assert.equal(f.e.blu_pair_bindkey.value,'');assert.equal(f.e.blu_pair_xiaomi_mac.value,'');
  await f.refresh({state:'waiting_xiaomi_beacon',candidates:[],error:''});
  assert.match(f.e.blu_pair_status.textContent,/encrypted Xiaomi/);f.hidden('pair_xiaomi_container');
  assert.match(f.e.blu_pair_help.textContent,/existing device assignment stays/);
  f=fixture([sensor(1371)]);f.e.blu_outdoor_type.value='xiaomi03';f.start(4);await tick();
  assert.equal(f.count('Pair'),0);f.hidden('pair_current_container');
  f.e.blu_pair_bindkey.value='e9ea895fac7cca6d30532432a516f3a8';
  f.e.blu_pair_bindkey.oninput();f.ctx.bluPairRefresh(f.c);await tick();assert.equal(f.count('Pair'),1);
  f=fixture([]);f.e.blu_indoor_type.value='xiaomi03';f.start(3);await tick();
  f.e.blu_pair_bindkey.value='e9ea895fac7cca6d30532432a516f3a8';await f.ctx.bluPairClose(f.c);
  assert.equal(f.e.blu_pair_bindkey.value,'');
  // Xiaomi contact works in both endpoint slots.
  for (const role of [0, 1]) {
    f=fixture([sensor(2443)]);
    if(role<2) f.e[role===0?'blu_closed_type':'blu_open_type'].value='xiaomicontact';
    f.start(role);await tick();assert.equal(f.count('Pair'),0);
    assert.equal(f.calls.find(call=>call.method==='Shelly.BLU.Discover').params.model,2443);
    f.hidden('pair_new_container');f.hidden('pair_current_container');f.visible('pair_xiaomi_container');
    f.e.blu_pair_bindkey.value='017e52d2684779298709b117c0a75a7b';
    f.ctx.bluPairRefresh(f.c);await tick();assert.equal(f.count('Pair'),1);
    await f.refresh({state:'waiting_xiaomi_beacon',candidates:[],error:''});
    assert.match(f.e.blu_pair_status.textContent,/contact state/);
    assert.match(f.e.blu_pair_help.textContent,/magnet/);
  }
  // Multiple candidates require an explicit selection, with no default first sensor.
  f=fixture([sensor(),sensor(2,false,'AA:BB:CC:DD:EE:08')]);f.start();f.ctx.bluPairConfirm(f.c);await tick();
  f.visible('pair_candidate_container');f.hidden('pair_new_container');assert.equal(f.count('Pair'),0);
  assert.equal(f.e.blu_pair_candidate.value,'');assert.equal(f.e.blu_pair_confirm.disabled,true);
  f.e.blu_pair_candidate.value='AA:BB:CC:DD:EE:08';f.e.blu_pair_candidate.onchange();
  f.ctx.bluPairConfirm(f.c);f.ctx.bluPairConfirm(f.c);await tick();assert.equal(f.count('Pair'),1);

  // Cancel waits for in-flight mutation RPCs, then cancels exactly once; no orphan pairing.
  for(const pendingMethod of ['Discover','Pair','SetPIN']) {
    f=fixture();const pending=deferred();f.handlers['Shelly.BLU.'+pendingMethod]=()=>pending.promise;
    f.start();f.ctx.bluPairConfirm(f.c);await tick();
    if(pendingMethod==='SetPIN') {
      await f.refresh({state:'waiting_encryption_pin',candidates:[],error:''});
      f.e.blu_pair_pin.value='135790';f.ctx.bluPairConfirm(f.c);await tick();
    }
    const close=f.ctx.bluPairClose(f.c);f.ctx.bluPairClose(f.c);await tick();
    assert.equal(f.e.blu_pairing.open,true);assert.equal(f.count('Cancel'),0);
    pending.resolve({});await close;await tick();
    assert.equal(f.count('Cancel'),1);assert.equal(f.e.blu_pairing.open,false);
    assert.equal(f.c.bluPairCredentials,null);assert.equal(f.e.blu_pair_pin.value,'');
  }
  // Late status responses cannot change a newly opened modal or its input.
  f=fixture();f.start();f.ctx.bluPairConfirm(f.c);await tick();
  let pending=deferred();f.handlers['Shelly.BLU.Status']=()=>pending.promise;
  f.ctx.bluPairRefresh(f.c);await f.ctx.bluPairClose(f.c);
  const nextDiscovery=deferred();f.handlers['Shelly.BLU.Discover']=()=>nextDiscovery.promise;f.start();
  pending.resolve({state:'scanning',candidates:[sensor(2,true)]});await tick();
  assert.equal(f.c.bluPairState,'starting');assert.equal(f.e.blu_pair_pin.value,'');
  assert.equal(f.e.blu_pair_current_container.open,false);
  f.hidden('pair_new_container');f.hidden('pair_current_container');

  // An old status poll must not reopen the PIN prompt after SetPIN was submitted.
  f=fixture();f.start();f.ctx.bluPairConfirm(f.c);await tick();
  await f.refresh({state:'waiting_encryption_pin',candidates:[],error:''});
  pending=deferred();f.handlers['Shelly.BLU.Status']=()=>pending.promise;
  f.ctx.bluPairRefresh(f.c);delete f.handlers['Shelly.BLU.Status'];
  f.e.blu_pair_pin.value='135790';f.ctx.bluPairConfirm(f.c);await tick();
  pending.resolve({state:'waiting_encryption_pin',candidates:[],error:''});await tick();
  assert.equal(f.c.bluPairState,'setting_pin');f.hidden('pair_confirm');f.hidden('pair_new_container');

  // RPC failures remain visible and never repeat Pair automatically.
  f=fixture();f.handlers['Shelly.BLU.Pair']=()=>Promise.reject(Error('Connection lost'));
  f.start();f.ctx.bluPairConfirm(f.c);await tick();f.ctx.bluPairRefresh(f.c);await tick();
  assert.equal(f.count('Pair'),1);assert.equal(f.c.bluPairState,'error');f.hidden('pair_new_container');
  await f.ctx.bluPairClose(f.c);assert.equal(f.count('Cancel'),1);
  // Failed cancellation leaves a clear retry action instead of pretending to have stopped.
  f=fixture();f.start();f.ctx.bluPairConfirm(f.c);await tick();
  f.handlers['Shelly.BLU.Cancel']=()=>Promise.reject(Error('Offline'));
  await f.ctx.bluPairClose(f.c);assert.equal(f.e.blu_pairing.open,true);
  assert.equal(f.e.blu_pair_cancel.textContent,'Retry cancel');
  delete f.handlers['Shelly.BLU.Cancel'];await f.ctx.bluPairClose(f.c);assert.equal(f.e.blu_pairing.open,false);
  console.log('PASS: automatic discovery/pairing without PIN, conditional legacy PIN and encryption activation, multi-device selection');
  console.log('PASS: automatic wake resume, duplicate clicks, stale polls and pending-RPC cancellation');
})().catch(e=>{console.error(e);process.exitCode=1;});
