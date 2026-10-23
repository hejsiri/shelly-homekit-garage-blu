// Preview adapter only: original firmware HTML/CSS/script.js are served unchanged.
const previewInfo = {
 name:'Shelly-Garage-BLU',model:'Shelly1Gen3',device_id:'Shelly1G3-DEMO00000001',
 version:'2.16.1 + BLU',fw_build:'20261022-171641',sys_mode:2,gdo_avail:true,wc_avail:false,
 hap_cn:1,hap_paired:false,hap_running:true,hap_ip_conns_pending:0,hap_ip_conns_active:0,hap_ip_conns_max:16,
 uptime:3600,sys_temp:39,auth_en:true,
 components:[{id:1,type:5,name:'Garage Door',cur_state:1,cur_state_str:'closed',move_time:20,pulse_time_ms:500,
 close_sensor_mode:0,open_sensor_mode:-1,out_mode:-1,sensor_swap:false,
 blu:{enable:true,closed_mac:'AA:BB:CC:DD:EE:01',open_mac:'AA:BB:CC:DD:EE:02',stale_after_s:180,ready:true,
 closed:{secure:true,seen:true,active:true,battery:92,rssi:-58,age_s:2},open:{secure:true,seen:true,active:false,battery:84,rssi:-65,age_s:3}}}]
};
refreshUI=function(){
 lastInfo=previewInfo;
 for(const [key,value] of Object.entries(previewInfo)) updateElement(key,value,previewInfo);
};
let previewPair = {state:"idle", error:"", candidates:[]};
let previewPairStart = 0;
callDevice=function(method,params){
 if(method==='Shelly.BLU.Discover') {
   previewPair={state:"scanning",error:"",candidates:[{mac:"AA:BB:CC:DD:EE:03",rssi:-48,encrypted:false}]};
   return Promise.resolve({});
 }
 if(method==='Shelly.BLU.Pair') {
   previewPair.state="connecting";previewPair.role=params.role;previewPair.mac=params.mac;
   previewPairStart=Date.now();return Promise.resolve({});
 }
 if(method==='Shelly.BLU.Cancel') {previewPair.state="error";previewPair.error="Preview: pairing cancelled";return Promise.resolve({});}
 if(method==='Shelly.BLU.Status') {
   if(previewPairStart && previewPair.state!=="done" && previewPair.state!=="error") {
     const stages=["connecting","securing_link","discovering_services","enabling_beacon","setting_pin","connecting_secure","reading_key","waiting_encrypted_beacon","done"];
     previewPair.state=stages[Math.min(8,Math.floor((Date.now()-previewPairStart)/1000))];
     if(previewPair.state==="done") {
       const role=previewPair.role===0?"closed":"open";
       previewInfo.components[0].blu[role+"_mac"]=previewPair.mac;
     }
   }
   return Promise.resolve(previewPair);
 }
 if(method==='Shelly.SetConfig' && params.id===1){
   Object.assign(previewInfo.components[0],params.config);
   refreshUI();return Promise.resolve({});
 }
 if(method==='Shelly.SetState' && params.id===1){
   const c=previewInfo.components[0];c.cur_state=c.cur_state===1?0:1;
   c.cur_state_str=c.cur_state===1?'closed':'open';
   c.blu.closed.active=c.cur_state===1;c.blu.open.active=c.cur_state===0;
   refreshUI();return Promise.resolve({});
 }
 return Promise.reject(new Error('Podgląd panelu: ta operacja wymaga fizycznego Shelly.'));
};
onLoad=function(){
 const previewStyles=document.querySelector('link[rel="stylesheet"]');
 previewStyles.href="style.css?preview="+Date.now();
 const banner=document.createElement('div');
 banner.textContent='PODGLĄD PANELU FIRMWARE · przykładowe dane · bez połączenia z Shelly';
 banner.style.cssText='padding:12px;text-align:center;background:#fff0bd;color:#302600;font:14px system-ui';
 document.body.prepend(banner);
 updateCommonVisibility(true);
 el('firmware_container').style.display='block';
 addInputChangeHandlers(document);refreshUI();setInterval(refreshUI,1000);
};
