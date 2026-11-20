const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const html=fs.readFileSync('fs_src/garage.html','utf8');
const start=html.indexOf('(()=>{\nconst leaf='),end=html.indexOf('\n})();',start)+6;
class Node {
  constructor(){this.textContent='';this.innerHTML='';this.hidden=false;this.children={};this.attributes={};this.classes=new Set();this.style={setProperty(){}};
    this.classList={toggle:(c,on)=>on?this.classes.add(c):this.classes.delete(c),add:c=>this.classes.add(c),remove:c=>this.classes.delete(c)};}
  querySelector(s){return this.children[s]||(this.children[s]=new Node());}
  querySelectorAll(s){return this.children[s]||(this.children[s]=Array.from({length:4},()=>new Node()));}
  setAttribute(k,v){this.attributes[k]=v;}
  closest(){return scene;}
  addEventListener(){}
}
let time=1000;
const nodes={},scene=new Node(),motion=new Node(),parent={},listeners={};
const node=id=>nodes[id]||(nodes[id]=new Node());
const ctx={car:'empty',apply(){ctx.renderCarPresence();},document:{getElementById:node,querySelectorAll:()=>[motion]},window:{addEventListener:(t,f)=>(listeners[t]??=[]).push(f)},parent,location:{origin:'http://shelly'},performance:{now:()=>time},setInterval(fn){ctx.watchdog=fn;},clearTimeout(){},setTimeout(){},cancelAnimationFrame(){},requestAnimationFrame(){return 1;}};
ctx.document.addEventListener=()=>{};
ctx.document.querySelector=selector=>node(selector);
ctx.layoutGarageReadings=()=>{};
ctx.setGarageDashboardVisibility=()=>{};
ctx.setGarageReadingValue=(node,text)=>{node.textContent=text;};
const originalQuery=Node.prototype.querySelector;
Node.prototype.querySelector=function(selector){const result=originalQuery.call(this,selector);if(!result.previousElementSibling)result.previousElementSibling=new Node();return result;};
ctx.document.querySelector=selector=>node('reading-values').querySelector(selector);
parent.postMessage=()=>{};vm.createContext(ctx);
vm.runInContext(html.slice(html.indexOf('let lastPresenceMAC='),html.indexOf('const assetCache=')),ctx);
vm.runInContext(html.slice(start,end),ctx);
const unsafe='<img src=x onerror=alert(1)>',status={name:'Garaż główny',cur_state_str:'stopped',obstruction_detected:true,movement_timed_out:true,blu:{enable:true,ready:true,stale_after_s:180,presence_timeout_s:45}};
for(const role of ['closed','open','motion','indoor','outdoor','button'])status.blu[role]={name:unsafe,secure:true,seen:true,age_s:0,active:role==='closed',temperature:21.5,humidity:40};
function send(origin='http://shelly'){for(const listener of listeners.message)listener({source:parent,origin,data:{garageTelemetry:{status}}});}
send('http://untrusted');assert.equal(node('status').textContent,'Loading…');
send();assert.equal(node('status').textContent,'Obstruction');assert(node('garage-control').classes.has('obstructed'));
assert.equal(node('leaf').style.transform,'translateY(-50%)');assert.equal(node('position').textContent,'50%');
assert.equal(node('garage-control').querySelector('.tile-name').textContent,'Garaż główny');
for(const id of ['summary-temperature','summary-humidity']){
  assert(node(id).title.includes(unsafe));
  assert(!node(id).innerHTML.includes(unsafe)); // User names never enter markup.
}
assert.equal(node('button-indicator').querySelector('.tile-name').textContent,unsafe);
assert.equal(node('car-presence').querySelector('.home-summary-label').textContent,'Car presence');
ctx.car='empty';ctx.apply();assert(!node('car-presence').classes.has('active'));
assert.equal(node('car-presence').querySelector('[data-reading="car"]').textContent,'Not detected');
send();assert(!node('car-presence').classes.has('active')); // Polling the same beacon preserves manual hide.
status.blu.button.packet_seq=1;send();assert(node('car-presence').classes.has('active'));
assert.equal(node('car-presence').querySelector('[data-reading="car"]').textContent,'Detected');
status.blu.car_name='Auto „garaż” '+unsafe;send();
assert.equal(node('car-presence').querySelector('.home-summary-label').textContent,status.blu.car_name);
assert.equal(node('car-presence').innerHTML,'');
assert.equal(node('button-indicator').querySelector('.tile-name').textContent,unsafe);
status.blu.car_name='';status.blu.button.name='Button';send();
assert.equal(node('car-presence').querySelector('.home-summary-label').textContent,'Car presence');
assert.equal(node('button-indicator').querySelector('.tile-name').textContent,'Button');
for(const state of ['open','closed']){
  status.cur_state_str=state;status.obstruction_detected=false; // HomeKit flag wins over a legacy timeout field.
  send();assert.equal(node('status').textContent,state==='open'?'Open':'Closed');
  assert(!node('garage-control').classes.has('obstructed'));
  assert.equal(node('position').textContent,'');
}
status.blu.ready=false;send();assert.equal(node('status').textContent,'No data');assert(node('garage-control').disabled);
console.log('PASS: saved names render as text on tiles and weather readings; dashboard uses HomeKit obstruction and endpoint state');

// Real signal bars and radio state, including loss of telemetry.
function connections(network){for(const listener of listeners.message)listener({source:parent,origin:'http://shelly',data:{garageTelemetry:{status,network}}});}
for(const [rssi,count] of [[-40,4],[-55,4],[-56,3],[-67,3],[-68,2],[-75,2],[-76,1],[-95,1]]){
 connections({wifi_connected:true,wifi_rssi:rssi,bt_enabled:true,bt_running:true});
 assert.equal(node('wifi-indicator').querySelectorAll('.wifi-arc').filter(n=>n.classes.has('active')).length,count);
 assert(node('wifi-indicator').attributes['aria-label'].includes(rssi+' dBm'));
 assert.equal(node('bt-indicator').hidden,false);
}
connections({wifi_connected:false,wifi_rssi:-40,bt_enabled:false,bt_running:false});
assert(node('wifi-indicator').classes.has('offline'));assert(node('bt-indicator').hidden);
connections({wifi_connected:true,wifi_rssi:null,bt_enabled:true,bt_running:false});
assert.equal(node('wifi-indicator').querySelectorAll('.wifi-arc').filter(n=>n.classes.has('active')).length,0);
assert(node('bt-indicator').classes.has('starting'));
time=17001;ctx.watchdog();assert(node('wifi-indicator').classes.has('offline'));assert(node('bt-indicator').hidden);
console.log('PASS: Wi-Fi RSSI thresholds, invalid values, actual Bluetooth radio state and stale telemetry');
