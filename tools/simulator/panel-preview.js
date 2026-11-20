// Preview adapter only: original firmware HTML/CSS/script.js are served unchanged.
const previewInfo = {
 name:'Shelly-Garage-BLU',model:'Shelly1Gen3',device_id:'Shelly1G3-DEMO00000001',
 version:'2.16.1_blu.87',fw_build:'20261027-235635',sys_mode:2,gdo_avail:true,wc_avail:false,
 hap_cn:1,hap_paired:false,hap_running:true,hap_ip_conns_pending:0,hap_ip_conns_active:0,hap_ip_conns_max:16,
 uptime:3600,sys_temp:39,auth_en:true,wifi_connected:true,wifi_conn_rssi:-58,
 components:[{id:1,type:5,name:'Garage door',cur_state:1,cur_state_str:'closed',move_time:20,pulse_time_ms:500,
 close_sensor_mode:0,open_sensor_mode:-1,out_mode:-1,sensor_swap:false,resume_after_stop:false,
 blu:{radio_enabled:true,radio_running:true,enable:true,closed_mac:'AA:BB:CC:DD:EE:01',open_mac:'AA:BB:CC:DD:EE:02',stale_after_s:180,presence_timeout_s:45,button_press_seq:0,button_press_age_ms:-1,ready:true,button_mac:'AA:BB:CC:DD:EE:07',button_name:'',car_name:'',button:{name:'Button',packet_seq:1,secure:true,seen:true,battery:93,rssi:-60,age_s:3},
 closed:{name:'Closing sensor',packet_seq:1,secure:true,seen:true,active:true,battery:92,rssi:-58,age_s:2},open:{name:'Opening sensor',packet_seq:1,secure:true,seen:true,active:false,battery:84,rssi:-65,age_s:3},motion_mac:'AA:BB:CC:DD:EE:04',motion:{name:'Motion sensor',packet_seq:1,secure:true,seen:true,active:false,battery:88,rssi:-51,age_s:0},indoor_mac:'AA:BB:CC:DD:EE:05',indoor:{name:'Indoor',packet_seq:1,secure:true,seen:true,temperature:21.4,humidity:48,battery:94,rssi:-55,age_s:12},outdoor_mac:'AA:BB:CC:DD:EE:06',outdoor:{name:'Outdoor',packet_seq:1,secure:true,seen:true,temperature:8.7,humidity:76,battery:91,rssi:-72,age_s:22}}}]
};
// Sample readings for the complete Home status strip; no physical device is used.
Object.assign(previewInfo.components[0].blu.motion,{illuminance:25,illuminance_ready:true,illuminance_age_s:0});
previewInfo.components[0].blu.smoke_mac='AA:BB:CC:DD:EE:08';
previewInfo.components[0].blu.smoke={paired:true,secure:true,seen:true,ready:true,smoke_seen:true,smoke_age_s:0,active:false,name:'Smoke sensor',battery:95,age_s:0};
const previewDashboardKey='garage-preview-dashboard';
let previewDashboard={};
try{previewDashboard=JSON.parse(localStorage.getItem(previewDashboardKey)||'{}')||{};}catch(_){}
for(const role of ['closed','open','motion','indoor','outdoor','button','smoke'])previewInfo.components[0].blu[role].dashboard=previewDashboard[role]!==false;

const previewHomeKitKey='garage-preview-homekit';
let previewHomeKit={};
try{previewHomeKit=JSON.parse(localStorage.getItem(previewHomeKitKey)||'{}')||{};}catch(_){}
for(const role of ['closed','open','motion','indoor','outdoor','button','smoke'])previewInfo.components[0].blu[role].homekit=previewHomeKit[role]!==false;

let previewBeaconAt = Date.now();
let previewMoveStart = 0, previewPosition = 0, previewMoveFrom = 0;
function advancePreview(){
 const c=previewInfo.components[0];
 if(!previewMoveStart)return;
 c.state_age_ms=Date.now()-previewMoveStart;
 if(c.cur_state!==2&&c.cur_state!==3)return;
 const delta=c.state_age_ms/(c.move_time*1000);
 previewPosition=Math.max(0,Math.min(1,previewMoveFrom+(c.cur_state===2?delta:-delta)));
 if((c.cur_state===2&&previewPosition===1)||(c.cur_state===3&&previewPosition===0)){
   c.cur_state=previewPosition===1?0:1;c.cur_state_str=previewPosition===1?'open':'closed';
   c.blu.closed.active=previewPosition===0;c.blu.open.active=previewPosition===1;previewMoveStart=0;
 }
}
refreshUI=function(){
 const c=previewInfo.components[0];
 advancePreview();
 const age = Math.floor((Date.now()-previewBeaconAt)/1000);
 c.blu.button.age_s = age;
 if(age>=10){
   c.blu.button.packet_seq++;c.blu.button.age_s=0;previewBeaconAt=Date.now();
   for(const role of ['closed','open','motion','indoor','outdoor','smoke']){
     const sensor=c.blu[role];
     if(!sensor||!(sensor.paired??sensor.secure))continue;
     sensor.packet_seq=(sensor.packet_seq||0)+1;sensor.age_s=0;
   }
   c.blu.motion.illuminance_age_s=0;c.blu.smoke.smoke_age_s=0;
 }
 c.blu.car_presence_ready=true;c.blu.car_present=!!(c.blu.button.secure&&c.blu.button.seen&&c.blu.button.age_s<=c.blu.presence_timeout_s);
 lastInfo=previewInfo;
 garageDashboardSetAccess(true);
 for(const [key,value] of Object.entries(previewInfo)) updateElement(key,value,previewInfo);
};
let previewPair = {state:"idle", error:"", candidates:[]};
let previewPairStart = 0;
let previewDiscoveryStart = 0;
callDevice=function(method,params){
 if(method==='Shelly.BLU.Bind') {
   const key=['closed','open','motion','indoor','outdoor','button','smoke'][params.role];
   const blu=previewInfo.components[0].blu;
   blu[key+'_mac']=params.mac;blu[key]={...blu[key],paired:true,secure:true,protocol:[839,982,2691].includes(previewPair.model)?'qingping_bindkey':'xiaomi_bindkey',seen:true};
   if(previewPair.model===5860) Object.assign(blu[key],{protocol:'xiaomi_clock_bindkey',ready:true,temperature_ready:true,humidity_ready:true,temperature:22.4,humidity:48,battery:92,rssi:-48,age_s:0});
   if(key==='smoke')Object.assign(blu.smoke,{seen:false,ready:false,active:false});
   previewPair={state:'done',error:'',candidates:[]};return Promise.resolve({});
 }
 if(method==='Shelly.BLU.Unpair') {
   const key=['closed','open','motion','indoor','outdoor','button','smoke'][params.role];
   const blu=previewInfo.components[0].blu;
   const dashboard=blu[key]&&blu[key].dashboard!==false;
   blu[key+'_mac']='';blu[key]={paired:false,secure:false,seen:false,ready:false,name:'',dashboard};
   if(params.role<2) blu.ready=false;
   if(params.role===5) {blu.button_name='';blu.car_name='';}
   return Promise.resolve({});
 }
 if(method==='Shelly.BLU.Discover') {
   previewPairStart=0;
   previewDiscoveryStart=Date.now();
   previewPair={state:"scanning",error:"",candidates:[{mac:"AA:BB:CC:DD:EE:03",rssi:-48,encrypted:false,model:2},{mac:"AA:BB:CC:DD:EE:04",rssi:-51,encrypted:false,model:5},{mac:"AA:BB:CC:DD:EE:05",rssi:-52,encrypted:false,model:3},{mac:"AA:BB:CC:DD:EE:07",rssi:-50,encrypted:false,model:1}]};
   previewPair.model=params.model||0;
   if(params.model===5860) previewPair.candidates=[{mac:"A4:C1:38:11:22:33",rssi:-48,encrypted:!!params.bindkey,model:9538}];
   if(params.model===1371) previewPair.candidates=[{mac:"A4:C1:38:AA:EA:0D",rssi:-48,encrypted:false,model:1371}];
   if(params.model===426) previewPair.candidates=[{mac:"58:2D:34:3B:2E:BF",rssi:-55,encrypted:false,model:426}];
   if(params.model===2455) previewPair.candidates=[{mac:'54:EF:44:E3:9C:BC',rssi:-54,encrypted:true,model:2455}];
   if(params.model===2443) previewPair.candidates=[{mac:'E4:AA:EC:81:21:AE',rssi:-48,encrypted:true,model:2443}];
   if([839,982,2691].includes(params.model)) previewPair.candidates=[{mac:'58:2D:34:11:22:33',rssi:-49,encrypted:false,model:params.model}];
   return Promise.resolve({});
 }
 if(method==='Shelly.BLU.Pair') {
   if([839,982,2691].includes(previewPair.model)) {
     const role=['closed','open','motion','indoor','outdoor','button','smoke'][params.role];
     const blu=previewInfo.components[0].blu;
     blu[role+'_mac']=params.mac;blu[role]={...blu[role],paired:true,secure:false,protocol:'qingping',seen:true};
     previewPair={state:'done',error:'',candidates:[]};return Promise.resolve({});
   }
   previewPair.state="connecting";previewPair.role=params.role;previewPair.mac=params.mac;
   previewPairStart=Date.now();return Promise.resolve({});
 }
 if(method==='Shelly.BLU.Cancel') {previewPairStart=0;previewPair.state="error";previewPair.error="Preview: pairing cancelled";return Promise.resolve({});}
 if(method==='Shelly.BLU.Resume') {previewPair.state="connecting_secure";previewPairStart=Date.now();return Promise.resolve({});}
 if(method==='Shelly.BLU.Status') {
   if(previewPair.state==='scanning' && Date.now()-previewDiscoveryStart<3000)
     return Promise.resolve({...previewPair,candidates:[]});
   if(previewPairStart && previewPair.state!=="done" && previewPair.state!=="error") {
     const stages=[5860,1371].includes(previewPair.model)?["reading_xiaomi","reading_xiaomi","done"]:previewPair.model===426?["waiting_beacon","waiting_beacon","done"]:["connecting","securing_link","discovering_services","enabling_beacon","reading_key","waiting_encrypted_beacon","done"];
     previewPair.state=stages[Math.min(stages.length-1,Math.floor((Date.now()-previewPairStart)/1000))];
     if(previewPair.state==="done") {
       const role=["closed","open","motion","indoor","outdoor","button","smoke"][previewPair.role];
       previewInfo.components[0].blu[role+"_mac"]=previewPair.mac;
       if(previewPair.model===426) Object.assign(previewInfo.components[0].blu[role],{protocol:'xiaomi',paired:true,secure:false,ready:true,temperature_ready:true,humidity_ready:true,seen:true,temperature:18,humidity:66,battery:59,rssi:-55,age_s:0});
       if(previewPair.model===5860) Object.assign(previewInfo.components[0].blu[role],{protocol:"xiaomi_clock_gatt",paired:true,secure:false,ready:true,temperature_ready:true,humidity_ready:true,seen:true,temperature:22.4,humidity:48,battery:-1,rssi:-48,voltage_mv:-1,age_s:0});
       if(previewPair.model===1371) Object.assign(previewInfo.components[0].blu[role],{protocol:'xiaomi_gatt',paired:true,secure:false,ready:true,temperature_ready:true,humidity_ready:true,seen:true,temperature:21.6,humidity:48,battery:90,battery_estimated:true,voltage_mv:3000,timeout_s:900,rssi:-48,age_s:0,gatt:{firmware:'2.1.1_0159',hardware:'B1.4',error:'',poll_interval_s:300}});
       if(role==='motion') Object.assign(previewInfo.components[0].blu.motion,{secure:true,seen:true,active:false,battery:88,rssi:-51,age_s:0});
     }
   }
   return Promise.resolve(previewPair);
 }
 if(method==='Shelly.SetConfig' && params.id===1){
   const c=previewInfo.components[0],{blu,...config}=params.config;
   Object.assign(c,config);
   if(blu){
     for(const [key,value] of Object.entries(blu)){
       const role=key.replace(/_name$/, '');
       const dashboardRole=key.replace(/_dashboard$/, '');
       if(key.endsWith('_dashboard')&&c.blu[dashboardRole]&&typeof value==='boolean'){
         c.blu[dashboardRole].dashboard=value;previewDashboard[dashboardRole]=value;
         try{localStorage.setItem(previewDashboardKey,JSON.stringify(previewDashboard));}catch(_){}
       }
       else if(key.endsWith('_homekit')&&c.blu[key.slice(0,-8)]&&typeof value==='boolean'){
         const homekitRole=key.slice(0,-8);
         c.blu[homekitRole].homekit=value;previewHomeKit[homekitRole]=value;
         try{localStorage.setItem(previewHomeKitKey,JSON.stringify(previewHomeKit));}catch(_){}
       }
       else if(key==='button_name') {c.blu.button_name=value;c.blu.button.name=value||'Button';}
       else if(key.endsWith('_name')&&c.blu[role]) c.blu[role].name=value;
       else c.blu[key]=value;
     }
   }
   refreshUI();return Promise.resolve({});
 }
 if(method==='Shelly.SetState' && params.id===1){
   advancePreview();
   const c=previewInfo.components[0];
   if(c.cur_state===2||c.cur_state===3){c.previous_direction=c.cur_state;c.cur_state=4;}
   else if(c.cur_state===4){const prev=c.previous_direction||2;c.cur_state=c.resume_after_stop?prev:prev===2?3:2;}
   else c.cur_state=c.cur_state===1?2:3;
   c.cur_state_str=c.cur_state===2?'opening':c.cur_state===3?'closing':'stopped';
   c.state_age_ms=0;previewMoveFrom=previewPosition;previewMoveStart=Date.now();
   c.blu.closed.active=false;c.blu.open.active=false;
   refreshUI();return Promise.resolve({});
 }
 return Promise.reject(new Error('Panel preview: this action requires a physical Shelly device.'));
};
onLoad=function(){
 if(new URLSearchParams(location.search).get('lang')==='pl'&&window.GarageI18n)GarageI18n.setLanguage('pl');
 const previewStyles=document.querySelector('link[rel="stylesheet"]');
 previewStyles.href="style.css?preview="+Date.now();
 const banner=document.createElement('div');
 banner.textContent='FIRMWARE PANEL PREVIEW · sample data · not connected to Shelly';
 banner.style.cssText='padding:12px;text-align:center;background:#fff0bd;color:#302600;font:14px system-ui';
 document.body.prepend(banner);
 updateCommonVisibility(true);
 el('firmware_container').style.display='block';
 addInputChangeHandlers(document);refreshUI();setInterval(refreshUI,1000);
};
