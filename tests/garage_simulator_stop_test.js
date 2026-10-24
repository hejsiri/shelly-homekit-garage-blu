const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
let now=100000;const ctx={Promise,Date:{now:()=>now},updateElement:()=>{},garageDashboardSetAccess:allowed=>assert.equal(allowed,true)};vm.createContext(ctx);vm.runInContext(fs.readFileSync('tools/simulator/panel-preview.js','utf8'),ctx);
const state=()=>vm.runInContext('previewInfo.components[0].cur_state',ctx);
const press=()=>ctx.callDevice('Shelly.SetState',{id:1,state:{toggle:true}});
press();assert.equal(state(),2);now+=5000;press();assert.equal(state(),4);press();assert.equal(state(),3);
now+=4999;ctx.refreshUI();assert.equal(state(),3);now++;ctx.refreshUI();assert.equal(state(),1);
press();now+=5000;press();ctx.callDevice('Shelly.SetConfig',{id:1,config:{resume_after_stop:true}});press();assert.equal(state(),2);
now+=14999;ctx.refreshUI();assert.equal(state(),2);now++;ctx.refreshUI();assert.equal(state(),0);
console.log('PASS: simulator reverses from 25% in 5s and continues remaining 75% in 15s');
