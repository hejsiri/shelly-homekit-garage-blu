const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const html=fs.readFileSync('fs_src/garage.html','utf8');
const start=html.indexOf('(()=>{\nconst leaf='),end=html.indexOf('\n})();',start)+6;
class Node {
  constructor(){this.textContent='';this.innerHTML='';this.hidden=false;this.children={};this.attributes={};this.classes=new Set();this.style={setProperty(){}};
    this.classList={toggle:(c,on)=>on?this.classes.add(c):this.classes.delete(c),add:c=>this.classes.add(c),remove:c=>this.classes.delete(c)};}
  querySelector(s){return this.children[s]||(this.children[s]=new Node());}
  setAttribute(k,v){this.attributes[k]=v;}
  closest(){return scene;}
  addEventListener(){}
}
const nodes={},scene=new Node(),motion=new Node(),parent={},listeners={};
const node=id=>nodes[id]||(nodes[id]=new Node());
const ctx={document:{getElementById:node,querySelectorAll:()=>[motion]},window:{addEventListener:(t,f)=>listeners[t]=f},parent,location:{origin:'http://shelly'},performance:{now:()=>1000},setInterval(){},clearTimeout(){},setTimeout(){},cancelAnimationFrame(){},requestAnimationFrame(){return 1;}};
parent.postMessage=()=>{};vm.createContext(ctx);vm.runInContext(html.slice(start,end),ctx);
const unsafe='<img src=x onerror=alert(1)>',status={name:'Garaż główny',cur_state_str:'stopped',obstruction_detected:true,movement_timed_out:true,blu:{enable:true,ready:true,stale_after_s:180,presence_timeout_s:45}};
for(const role of ['closed','open','motion','indoor','outdoor','button'])status.blu[role]={name:unsafe,secure:true,seen:true,age_s:0,active:role==='closed',temperature:21.5,humidity:40};
function send(origin='http://shelly'){listeners.message({source:parent,origin,data:{garageTelemetry:{status}}});}
send('http://untrusted');assert.equal(node('status').textContent,'Loading…');
send();assert.equal(node('status').textContent,'Obstruction');assert(node('garage-control').classes.has('obstructed'));
assert.equal(node('leaf').style.transform,'translateY(-50%)');assert.equal(node('position').textContent,'50%');
assert.equal(node('garage-control').querySelector('.tile-name').textContent,'Garaż główny');
assert.equal(motion.querySelector('.tile-name').textContent,unsafe);
for(const id of ['weather-indoor','weather-outdoor']){
  assert.equal(node(id).querySelector('.sensor-label').textContent,unsafe+':');
  assert(!node(id).innerHTML.includes(unsafe)); // User names never enter markup.
}
for(const id of ['button-indicator','car-presence'])assert.equal(node(id).querySelector('.tile-name').textContent,unsafe);
for(const state of ['open','closed']){
  status.cur_state_str=state;status.obstruction_detected=false; // HomeKit flag wins over a legacy timeout field.
  send();assert.equal(node('status').textContent,state==='open'?'Open':'Closed');
  assert(!node('garage-control').classes.has('obstructed'));
  assert.equal(node('position').textContent,'');
}
status.blu.ready=false;send();assert.equal(node('status').textContent,'No data');assert(node('garage-control').disabled);
console.log('PASS: saved names render as text on tiles and weather readings; dashboard uses HomeKit obstruction and endpoint state');
