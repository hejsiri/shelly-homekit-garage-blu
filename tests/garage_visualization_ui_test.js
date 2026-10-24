// Test shared motion estimation independently of the removed legacy SVG.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync('fs_src/script.js','utf8');let time=10000;
const ctx={performance:{now:()=>time}};vm.createContext(ctx);vm.runInContext(source.slice(source.indexOf('function garageRefresh('),source.indexOf('// Fixed markup')),ctx);
const c={},cd={cur_state_str:'opening',move_time:30,state_age_ms:15000};
const draw=data=>ctx.garageRefresh(c,data),motion=()=>c.garageMotionState.motion;
draw(cd);assert.equal(motion().from,.5);assert.equal(motion().duration,15000);
const old=motion();time+=1000;draw({...cd,state_age_ms:16000});assert.equal(motion(),old);
draw({...cd,cur_state_str:'open'});assert.equal(motion().to,1);assert.equal(motion().duration,0);
draw({...cd,cur_state_str:'closing',state_age_ms:0});time+=5000;draw({...cd,cur_state_str:'stopped',state_age_ms:0});assert(motion().to>0&&motion().to<1);
draw({...cd,blu:{enable:true,ready:false}});assert.equal(motion().duration,0);
assert(!fs.readFileSync('fs_src/index.html','utf8').includes('id="garage_view"'));
console.log('PASS: mid-travel reload, polling continuity, endpoints, stopping, unavailable state; legacy SVG removed');

// Constant speed after stopping, reversing and continuing from a partial position.
const partial={};time=100000;
ctx.garageRefresh(partial,{cur_state_str:'closed',move_time:20,state_age_ms:0});
ctx.garageRefresh(partial,{cur_state_str:'opening',move_time:20,state_age_ms:0});
time+=5000;ctx.garageRefresh(partial,{cur_state_str:'stopped',move_time:20,state_age_ms:0});
assert.equal(partial.garageMotionState.motion.from,.25);
ctx.garageRefresh(partial,{cur_state_str:'closing',move_time:20,state_age_ms:0});
assert.equal(partial.garageMotionState.motion.duration,5000);
time+=2000;ctx.garageRefresh(partial,{cur_state_str:'stopped',move_time:20,state_age_ms:0});
assert(Math.abs(partial.garageMotionState.motion.from-.15)<1e-9);
ctx.garageRefresh(partial,{cur_state_str:'opening',move_time:20,state_age_ms:0});
assert.equal(partial.garageMotionState.motion.duration,17000);
console.log('PASS: 25% reversal takes 5s of a 20s full travel; continuation keeps same speed');
