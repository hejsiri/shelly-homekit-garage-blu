const fs = require('fs'), vm = require('vm'), assert = require('assert/strict');
const source = fs.readFileSync('fs_src/script.js', 'utf8');
const upload = source.slice(source.indexOf('async function uploadFW('), source.indexOf('// major.minor.patch'));
async function run(fail = false, timeout = false) {
  const calls = [], status = {}, spinner = {}, changed = [];
  let abortTimer, cleared = false;
  const ctx = {FormData, Headers, AbortController, setUpdateInProgress(){},
    setTimeout(fn){abortTimer=fn;return 1}, clearTimeout(){cleared=true},
    setVar(...args){changed.push(args)},
    authHeaderToReq(method, uri, challenge){assert.equal(method,'POST');assert.equal(uri,'/update');assert.equal(challenge,'digest');return {}},
    getAuthResp(){return {httpAuth:'test-authorization'}},
    async fetch(url, options){
      calls.push(options);
      if(calls.length===1) {assert.equal(options.body,undefined);return {status:401,headers:new Headers({'www-authenticate':'digest'})}}
      assert.equal(options.headers.get('Authorization'),'test-authorization');
      assert(options.body instanceof FormData);
      if(timeout) return new Promise((resolve,reject)=>options.signal.addEventListener('abort',()=>reject(Object.assign(Error(),{name:'AbortError'}))));
      return {ok:!fail,status:fail?403:200,async text(){return fail?'Denied':'OK'}};
    }};
  vm.createContext(ctx);vm.runInContext(upload,ctx);
  const pending=ctx.uploadFW(new Blob(['firmware']),spinner,status);
  await new Promise(setImmediate);
  if(timeout) abortTimer();
  await pending;
  assert.equal(calls.length,2);assert(cleared);assert.equal(spinner.className,'');
  if(timeout) {assert.match(status.innerText,/timed out/);assert.equal(changed.length,0)}
  else if(fail){assert.match(status.innerText,/Denied/);assert.equal(changed.length,0)}
  else {assert.equal(status.innerText,'OK');assert.deepEqual(changed,[['update_available',false]])}
}
(async()=>{await run();await run(true);await run(false,true);console.log('PASS: OTA authenticates before ZIP, awaits response, reports errors and timeout without automatic retry')})().catch(e=>{console.error(e);process.exit(1)});
