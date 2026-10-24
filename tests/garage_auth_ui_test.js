// Exercise production authentication callbacks and delayed GetInfoExt responses.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync('fs_src/script.js','utf8');
const tick=()=>new Promise(setImmediate);
function section(a,b){return source.slice(source.indexOf(a),source.indexOf(b,source.indexOf(a)));}
function fixture(){
  const elements={},requests=[],attrs={'data-src':'garage.html'};
  let reloads=0;
  const el=id=>elements[id]||(elements[id]={style:{display:'none'},focus(){},getAttribute:n=>attrs[n]||null,
    setAttribute:(n,v)=>{attrs[n]=v},removeAttribute:n=>{delete attrs[n]}});
  const ctx={lastInfo:null,infoLevel:0,pendingGetInfo:false,updateInProgress:false,pauseAutoRefresh:false,
    pendingRequests:{},rpcAuth:null,authRealm:null,host:'shelly',isConnected:false,authInfoKey:'auth_info',maxAuthAge:100,
    el,updateElement(){},updateCommonVisibility(){},setVar(){},getAuthResp(){return null},setupHost(){},
    reloadPage(){reloads++},console:{log(){}},window:{addEventListener(){}},
    document:{documentElement:{dataset:{}}},location:{origin:'http://shelly'},performance:{now:()=>0},
    WebSocket:class{close(){}},
    callDevice(method){return new Promise((resolve,reject)=>requests.push({method,resolve,reject}))}};
  vm.createContext(ctx);
  vm.runInContext(source.slice(source.indexOf('let garageDashboardComponent = null')),ctx);
  vm.runInContext(section('function getInfo()','function getVar('),ctx);
  vm.runInContext(section('function connectWebSocket()','// Implementation of the digest'),ctx);
  vm.runInContext(section('el("sec_log_out_btn").onclick','el("sec_new_pass").onkeyup'),ctx);
  return {ctx,el,requests,attrs,get reloads(){return reloads},hidden(){assert.equal(el('header_container').style.display,'none');assert.equal(attrs.src,undefined)}};
}
const info={name:'Demo',model:'ShellyPlus1',device_id:'test',version:'test',fw_build:'test',auth_en:true,components:[]};
(async()=>{
  // Public basic info never authorizes the iframe; only successful extended info does.
  let f=fixture(),pending=f.ctx.getInfo();
  assert.equal(f.requests[0].method,'Shelly.GetInfo');f.requests.shift().resolve(info);await tick();f.hidden();
  assert.equal(f.requests[0].method,'Shelly.GetInfoExt');f.requests.shift().resolve(info);await pending;
  assert.equal(f.attrs.src,'garage.html');assert.equal(f.el('header_container').style.display,'block');
  // Logging out immediately unloads the dashboard and invalidates in-flight replies.
  f.ctx.rpcAuth={token:'test'};pending=f.ctx.getInfo();const late=f.requests.shift();
  f.el('sec_log_out_btn').onclick();f.hidden();assert.equal(f.ctx.rpcAuth,null);assert.equal(f.reloads,1);
  const rejected=assert.rejects(pending,/session changed/);late.resolve(info);await rejected;f.hidden();
  // A device without password protection remains accessible after GetInfoExt succeeds.
  f=fixture();f.ctx.infoLevel=1;pending=f.ctx.getInfo();f.requests.shift().resolve({...info,auth_en:false});
  await pending;assert.equal(f.attrs.src,'garage.html');
  // A denied extended-info request cannot expose the panel.
  f=fixture();f.ctx.infoLevel=1;pending=f.ctx.getInfo();
  const denied=assert.rejects(pending,/Denied/);f.requests.shift().reject(Error('Denied'));await denied;f.hidden();
  // A 401 with no usable credentials, or a rejected PIN/password response, revokes access.
  for(const loggedIn of [false,true])for(const triedAuth of [false,true]){
    f=fixture();if(loggedIn){f.ctx.lastInfo=info;f.ctx.garageDashboardSetAccess(true)}
    const connected=f.ctx.connectWebSocket();f.ctx.socket.onopen();await connected;
    let rejected401=false;
    f.ctx.pendingRequests[1]={method:'Shelly.GetInfoExt',ar:triedAuth?{}:null,reject(){rejected401=true}};
    f.ctx.socket.onmessage({data:JSON.stringify({id:1,error:{code:401,message:JSON.stringify({realm:'test'})}})});
    f.hidden();assert.equal(f.ctx.rpcAuth,null);
    if(loggedIn)assert.equal(f.reloads,1);else{assert(rejected401);assert.equal(f.el('auth_container').style.display,'block')}
  }
  // Losing the socket revokes previously confirmed access until the next full response.
  f=fixture();const connected=f.ctx.connectWebSocket();f.ctx.socket.onopen();await connected;
  f.ctx.garageDashboardSetAccess(true);f.ctx.socket.onclose({code:1006});f.hidden();
  console.log('PASS: public/full RPC distinction, login, no-password setup, 401, logout, late replies and disconnect');
})().catch(e=>{console.error(e);process.exitCode=1});
