const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const html=fs.readFileSync('fs_src/garage.html','utf8');
const sceneMarkup=html.slice(html.indexOf('<div class="house-canvas">'),html.indexOf('<aside'));
assert(!/\bsrc=/.test(sceneMarkup)); // No legacy/default layer is requested before the chosen theme.
assert(html.includes('.scene{visibility:hidden}html[data-scene-ready="true"] .scene{visibility:visible}'));
const source=html.slice(html.indexOf('let themeRequest='),html.indexOf("const scene=document.querySelector('.scene')"));
const images=[],nodes={};
const node=id=>nodes[id]||(nodes[id]={attributes:{},setAttribute(k,v){this.attributes[k]=v;},addEventListener(){},closest(){return node('scene');}});
class Image {
  set src(value){this.path=value;images.push(this);}
  decode(){return Promise.resolve();}
}
const ctx={Image,Map,Promise,console,mode:'night',car:'car',system:{matches:true,addEventListener(){}},base:node('interior'),exterior:node('exterior'),parent:{postMessage(){}},location:{origin:'http://shelly'},window:{addEventListener(){}},document:{documentElement:{dataset:{}},querySelector:node,querySelectorAll:()=>[],getElementById:node}};
vm.createContext(ctx);vm.runInContext(source,ctx);
(async()=>{
  const first=ctx.apply();assert.equal(images.length,4);
  assert(images.every(i=>i.path.includes('night')));
  assert.equal(ctx.exterior.src,undefined);assert.equal(ctx.document.documentElement.dataset.sceneReady,undefined);
  images.slice(0,3).forEach(i=>i.onload());await Promise.resolve();await Promise.resolve();
  assert.equal(ctx.document.documentElement.dataset.sceneReady,undefined);
  images[3].onload();await first;
  assert.equal(ctx.exterior.src,'garage-v16-night.webp');
  assert.equal(node('.car-layer').src,'garage-tesla-s-night.webp');
  assert.equal(ctx.document.documentElement.dataset.sceneReady,'true');
  // A slow previous theme must not overwrite the user's newer selection.
  ctx.mode='day';const slow=ctx.apply();assert.equal(images.length,8);
  ctx.mode='night';await ctx.apply();images.slice(4).forEach(i=>i.onload());await slow;
  assert.equal(ctx.exterior.src,'garage-v16-night.webp');
  assert.equal(ctx.document.documentElement.dataset.theme,'night');
  console.log('PASS: no default scene request; reveal only complete decoded theme; stale theme load cannot overwrite selection');
})().catch(e=>{console.error(e);process.exitCode=1;});
