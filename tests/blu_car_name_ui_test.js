// Exercise production config submission and polling with empty and independent labels.
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const script=fs.readFileSync('fs_src/script.js','utf8');
const functions=[
  script.slice(script.indexOf('function gdoSetConfig('),script.indexOf('\nel("reboot_btn")')),
  script.slice(script.indexOf('function updateComponent('),script.indexOf('\nfunction updateStaticIPVisibility(')),
  script.slice(script.indexOf('function setValueIfNotModified('),script.indexOf('\nfunction checkIfNotModified(')),
];
const fields={},field=id=>fields[id]||(fields[id]={value:'',checked:false,dataset:{},style:{}});
const c={},alerts=[],calls=[];
const ctx={TextEncoder,console,document:{activeElement:null},Component_Type:{kGarageDoorOpener:5},
  el:(_,id)=>field(id),findOrAddContainer:()=>c,addInputChangeHandlers(){},
  garageRefresh(){},garageDashboardRefresh(){},bluPairRefresh(){},bluSensorStatus(){},
  checkIfNotModified(){},selectIfNotModified(){},updateInnerText(){},alert:m=>alerts.push(m),
  setComponentConfig:(_,config)=>calls.push(JSON.parse(JSON.stringify(config)))};
vm.createContext(ctx);vm.runInContext(functions.join('\n'),ctx);
const cd={id:1,type:5,name:'Garage',cur_state_str:'closed',move_time:20,pulse_time_ms:500,
  open_sensor_mode:-1,out_mode:-1,blu:{enable:false,ready:true,closed_mac:'',open_mac:'',
  stale_after_s:180,button_name:'',car_name:'',button:{secure:true,name:'Button'}}};
ctx.updateComponent(cd);
assert.equal(field('blu_button_name').value,''); // Effective label must not replace placeholder.
assert.equal(field('blu_car_name').value,'');
assert.equal(field('blu_car_name_row').style.display,'');
field('blu_button_name').value='  Pilot garażu  ';field('blu_car_name').value='  Tesla Model S  ';
ctx.gdoSetConfig(c);assert.equal(alerts.length,0);
assert.equal(calls[0].blu.button_name,'Pilot garażu');assert.equal(calls[0].blu.car_name,'Tesla Model S');
field('blu_car_name').dataset.changed='true';ctx.updateComponent(cd);
assert.equal(field('blu_car_name').value,'  Tesla Model S  '); // Polling preserves unsaved edits.
field('blu_car_name').dataset.changed='false';
field('blu_car_name').value='ż'.repeat(33);ctx.gdoSetConfig(c);
assert.equal(calls.length,1);assert.match(alerts.pop(),/Car name is too long/);
field('blu_car_name').value='';field('blu_button_name').value='';ctx.gdoSetConfig(c);
assert.equal(calls[1].blu.button_name,'');assert.equal(calls[1].blu.car_name,'');
cd.blu.button.secure=false;ctx.updateComponent(cd);
assert.equal(field('blu_car_name_row').style.display,'none');ctx.gdoSetConfig(c);
assert(!('car_name' in calls[2].blu)); // Unpaired devices do not overwrite saved labels.
console.log('PASS: independent car/button names, empty placeholders, UTF-8 validation and polling-safe edits');
