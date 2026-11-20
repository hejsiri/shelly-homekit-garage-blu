const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const html=fs.readFileSync('fs_src/garage.html','utf8');
const values={},items={};
for(const key of ['temperature','humidity','light','open','closed','motion','smoke','car']){
  const label={textContent:'',setAttribute(){}},value={textContent:'',previousElementSibling:label};
  const item={hidden:key==='car',style:{},title:'',classList:{contains:name=>name==='home-summary',toggle(){}},querySelector:selector=>selector==='.home-summary-label'?label:value};
  value.closest=()=>item;values[key]=value;items[key]=item;
}
const groups=[{children:['temperature','humidity','light'].map(k=>items[k])},{children:['open','closed','motion','smoke','car'].map(k=>items[k])}];
const context={
  document:{querySelector:selector=>values[selector.match(/data-reading="([^"]+)"/)[1]],querySelectorAll:()=>groups,getElementById:id=>id==='car-presence'?items.car:items[id.replace('summary-','')]},
  GarageI18n:{t:text=>text,getLanguage:()=> 'en'},performance:{now:()=>0},car:'car',presenceUpdateAt:0,
  setGarageReadingValue(node,text){node.textContent=text;}
};
vm.createContext(context);
vm.runInContext(html.slice(html.indexOf('function layoutGarageReadings(){'),html.indexOf('const garageReadingUpdates=')),context);
vm.runInContext(html.slice(html.indexOf('function renderSummary(data){'),html.indexOf("document.addEventListener('garage-language-change',()=>")),context);
vm.runInContext(html.slice(html.indexOf('function renderCarPresence(){'),html.indexOf("document.addEventListener('garage-language-change',renderCarPresence)")),context);
const fresh={paired:true,secure:true,seen:true,ready:true,age_s:0,packet_seq:1};
const blu={stale_after_s:180,closed:{...fresh,active:true},open:{...fresh,active:false},motion:{...fresh,active:false,illuminance:25,illuminance_ready:true},indoor:{...fresh,temperature:21.4,humidity:48},outdoor:{...fresh,temperature:8.7,humidity:76},smoke:{...fresh,active:false,smoke_seen:true,smoke_age_s:0},button:{...fresh},car_presence_ready:true};
const render=()=>{context.presenceData=blu;context.renderCarPresence();context.renderSummary({status:{blu}});};
render();
assert.equal(values.temperature.textContent,'8.7–21.4°');assert.equal(values.humidity.textContent,'48–76%');
assert(Object.values(items).every(item=>!item.hidden));
assert.equal(items.smoke.style.gridColumn,'2 / span 2');assert.equal(items.car.style.gridColumn,'4 / span 2');
blu.closed.dashboard=false;blu.motion.dashboard=false;blu.indoor.dashboard=false;render();
assert(items.closed.hidden&&items.motion.hidden&&items.light.hidden);
assert(!items.open.hidden&&!items.smoke.hidden&&!items.car.hidden);
assert.equal(values.temperature.textContent,'8.7°');assert.equal(values.humidity.textContent,'76%');
assert.equal(items.temperature.style.gridColumn,'2 / span 2');assert.equal(items.humidity.style.gridColumn,'4 / span 2');
blu.outdoor.dashboard=false;render();assert(groups[0].hidden);
blu.button.dashboard=false;render();assert(items.car.hidden);
assert.equal(items.open.style.gridColumn,'2 / span 2');assert.equal(items.smoke.style.gridColumn,'4 / span 2');
blu.open.dashboard=false;blu.smoke.dashboard=false;render();assert(groups[1].hidden);
for(const role of ['closed','open','motion','indoor','outdoor','button','smoke'])blu[role].dashboard=true;
render();assert(groups.every(group=>!group.hidden));assert(Object.values(items).every(item=>!item.hidden));
assert.equal(values.temperature.textContent,'8.7–21.4°');
const schema=fs.readFileSync('mos.yml','utf8'),index=fs.readFileSync('fs_src/index.html','utf8');
for(const role of ['closed','open','motion','indoor','outdoor','button','smoke']){
  assert.equal(schema.split('["blu.'+role+'_dashboard", "b", true,').length-1,2);
  assert(index.includes('id="blu_'+role+'_dashboard" checked'));
  assert.equal(schema.split('["blu.'+role+'_homekit", "b", true,').length-1,2);
  assert(index.indexOf('id="blu_'+role+'_homekit"')>index.indexOf('id="blu_'+role+'_dashboard"'));
}
const storage=new Map(),preview=fs.readFileSync('tools/simulator/panel-preview.js','utf8');
const previewContext=()=>({localStorage:{getItem:key=>storage.get(key)||null,setItem:(key,value)=>storage.set(key,value)},garageDashboardSetAccess(){},updateElement(){}});
const session=previewContext();vm.createContext(session);vm.runInContext(preview,session);
(async()=>{
  await session.callDevice('Shelly.SetConfig',{id:1,config:{blu:{motion_dashboard:false,indoor_dashboard:false}}});
  assert.equal(vm.runInContext('previewInfo.components[0].blu.motion.dashboard',session),false);
  const reopened=previewContext();vm.createContext(reopened);vm.runInContext(preview,reopened);
  assert.equal(vm.runInContext('previewInfo.components[0].blu.motion.dashboard',reopened),false);
  assert.equal(vm.runInContext('previewInfo.components[0].blu.indoor.dashboard',reopened),false);
  assert.equal(vm.runInContext('previewInfo.components[0].blu.outdoor.dashboard',reopened),true);
  console.log('PASS: per-device visibility, selected H&T aggregation, centered hidden layouts, schema defaults and preview persistence');
})().catch(error=>{console.error(error);process.exitCode=1;});
