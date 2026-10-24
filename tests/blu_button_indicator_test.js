const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const html=fs.readFileSync('fs_src/garage.html','utf8');
const start=html.indexOf(" const indicator=document.getElementById('button-indicator');");
const end=html.indexOf(' lastButtonSequence=seq;',start)+' lastButtonSequence=seq;'.length;
assert(start>0&&end>start);
const active=new Set(),caption={textContent:'Ready'},indicator={hidden:false,classList:{add:c=>active.add(c),remove:c=>active.delete(c)},querySelector:()=>caption};
let timer,flashes=0;
const ctx={blu:{button:{secure:true},button_press_seq:0,button_press_age_ms:-1},lastButtonSequence:null,buttonPulseTimer:0,document:{getElementById:()=>indicator},clearTimeout:()=>{},setTimeout:f=>{timer=f;flashes++;return 1;}};
vm.createContext(ctx);const run=()=>vm.runInContext('{'+html.slice(start,end)+'}',ctx);
run();assert(!active.has('pulse'));
ctx.blu.button_press_seq=1;ctx.blu.button_press_age_ms=0;run();assert(active.has('pulse'));assert.equal(caption.textContent,'Pressed');assert.equal(flashes,1);
run();assert.equal(flashes,1);timer();assert(!active.has('pulse'));assert.equal(caption.textContent,'Ready');
run();assert.equal(flashes,1); // beacon/polling cannot retrigger the indicator
ctx.blu.button_press_seq=2;ctx.blu.button_press_age_ms=9000;run();assert.equal(flashes,1);
ctx.blu.button={secure:false};ctx.blu.button_press_seq=3;ctx.blu.button_press_age_ms=0;run();assert(indicator.hidden);assert.equal(flashes,1);
console.log('PASS: one flash per fresh authenticated press; no flash on beacon, repeated poll, stale press or unpaired device');
