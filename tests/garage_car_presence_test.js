const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const source=fs.readFileSync('fs_src/garage.html','utf8');
const start=source.indexOf('let lastPresenceMAC='),end=source.indexOf("window.addEventListener('message',e=>{",start);
const ctx={car:'empty',changes:0,apply(){this.changes++;}};
vm.createContext(ctx);vm.runInContext(source.slice(start,end),ctx);
const blu={button_mac:'AA:BB:CC:DD:EE:01',presence_timeout_s:45,button:{secure:true,seen:true,age_s:0,packet_seq:1}};
const update=()=>ctx.syncCarPresence(blu);
update();assert.equal(ctx.car,'car');
ctx.car='empty';blu.button.age_s=1;update();assert.equal(ctx.car,'empty'); // Same packet, new poll.
blu.button.packet_seq++;blu.button.age_s=0;update();assert.equal(ctx.car,'car');
blu.button.age_s=45;update();assert.equal(ctx.car,'car');
blu.button.age_s=46;update();assert.equal(ctx.car,'empty');
ctx.car='car';blu.button.age_s=47;update();assert.equal(ctx.car,'car'); // Manual override while absent.
blu.button.packet_seq++;blu.button.age_s=0;update();assert.equal(ctx.car,'car');
ctx.car='empty';blu.button.packet_seq++;update();assert.equal(ctx.car,'car'); // Next beacon wins again.
blu.button_mac='AA:BB:CC:DD:EE:02';blu.button.seen=false;update();assert.equal(ctx.car,'empty');
blu.button.secure=false;ctx.car='car';update();assert.equal(ctx.car,'car'); // No pairing: toolbar is independent.
blu.button.secure=true;blu.button.seen=true;blu.button.packet_seq=0;update();assert.equal(ctx.car,'car');
console.log('PASS: authenticated beacon overrides manual car selection; repeated polling preserves it; timeout hides car');
