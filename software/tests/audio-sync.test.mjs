import test from 'node:test';
import {initialPlayback,schedulePlayback} from '../lib/timing.mjs';
import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';

// Execute the actual demo script with deterministic DOM/audio/clock substitutes.
// This verifies control and sequencing logic, not real audio output or hardware.
const html=fs.readFileSync(new URL('../public/index.html',import.meta.url),'utf8');
let code=html.match(/<script>([\s\S]*?)<\/script>/)[1];
code=code.replace('queue.push({time:', 'auditEvents.push({time:nextTime,step:nextStep,hits:hits.slice(),loop,applied,partner:partner.slice()});queue.push({time:');
code=code.replace('queue.push({time:audioTime,', 'auditEvents.push({time:audioTime,serverAt:tick.at,absoluteStep:tick.step,frameRevision:frame.revision,hits:hits.slice()});queue.push({time:audioTime,');
code=code.replace(/\}\)\(\);\s*$/,`window.testHooks={scheduler,animate,stop,start,valid,share:(p)=>{session={playback:p,clock:{ready:true,offset:epoch-perfOrigin,errorMs:0,now:()=>epoch+performance.now()-perfOrigin,toServer:t=>t+epoch-perfOrigin,toPerformance:t=>t-epoch+perfOrigin},monotonic:()=>performance.now(),get lastContact(){return performance.now()-sessionAge();},syncClock:async()=>{},queue:[]};const f=p.frames.at(-1);receiveShared({pattern:f.pattern,partner:f.partner,bpm:f.bpm});},updatePlayback:p=>{session.playback=p;},get:()=>({pattern:clone(pattern),draft:draft.slice(),partner:partner.slice(),pending:pending?.slice(),playing,bpm,nextStep,nextTime,queue:queue.slice(),sources:activeSources.size})};})();`);
class Element {
 constructor(){this.children=[];this.listeners={};this.attrs={};this.dataset={};this.style={setProperty(){}};this.classList={add(){},remove(){}};this.value='';this.checked=false;this.hidden=false;this.disabled=false;}
 appendChild(e){this.children.push(e);return e;} setAttribute(k,v){this.attrs[k]=v;} addEventListener(k,fn){this.listeners[k]=fn;} async fire(k){return this.listeners[k]?.({target:this});}
}
function setup({begin=0,audioOrigin=0,latency=0,perfOrigin=0}={}){
 const saved=null;let contactAge=0;const epoch=100000;
 const els={},events=[],sources=[],store=new Map(saved?[['beatblocks-demo-v1',saved]]:[]);let clock=begin;
 const el=id=>els[id]??=(new Element());el('tempo').value='96';el('volume').value='55';
 const param=()=>({value:0,setValueAtTime(){},exponentialRampToValueAtTime(){},setTargetAtTime(){}});
 const node=kind=>({kind,gain:param(),frequency:param(),connect(){},disconnect(){},addEventListener(){},start(t){sources.push({kind,time:t});},stop(){}});
 class Audio {constructor(){this.state='running';this.sampleRate=48000;this.destination={};}get currentTime(){return clock-audioOrigin;}get outputLatency(){return latency;}getOutputTimestamp(){return {contextTime:clock-audioOrigin-latency,performanceTime:clock*1000+perfOrigin};}async resume(){}createGain(){return node('gain');}createBuffer(n,length){return{getChannelData(){return new Float32Array(length);}};}createOscillator(){return node('oscillator');}createBufferSource(){return node('noise');}createBiquadFilter(){return node('filter');}}
 const document={getElementById:el,createElement:()=>new Element(),listeners:{},addEventListener(k,v){this.listeners[k]=v;},hidden:false};
 const window={AudioContext:Audio,addEventListener(){}};
 const ctx={window,document,performance:{now:()=>clock*1000+perfOrigin},epoch,perfOrigin,sessionAge:()=>contactAge,auditEvents:events,localStorage:{getItem:k=>store.get(k)||null,setItem:(k,v)=>store.set(k,v)},setInterval:()=>1,clearInterval(){},requestAnimationFrame:()=>1,cancelAnimationFrame(){},console,Math,Array,Set,Number,JSON,Error};
 vm.runInNewContext(code,ctx);const h=window.testHooks;
 return{el,h,events,sources,document,store,contactAge:n=>contactAge=n,async click(id){await el(id).fire('click');},advance(seconds){const end=clock+seconds;while(clock<end-1e-8){clock=Math.min(end,clock+.005);h.scheduler();h.animate();}},async tempo(n){el('tempo').value=String(n);await el('tempo').fire('input');},async preset(p){el('preset').value=p;await el('preset').fire('change');},async local(r,s){await el('sequence').children[9+r*9+1+s].fire('click');}};
}

const seed={pattern:[[1,0,0,0,1,0,0,0],[0,0,1,0,0,0,1,0]],partner:[1,0,1,0,1,0,1,0],bpm:120};
const playback=()=>initialPlayback(seed,100000,0);
const sharedEvents=a=>a.events.filter(e=>e.serverAt!==undefined);
test('actual audio scheduler: late Play and distinct output delays target identical sound times',async()=>{
 const a=setup({latency:.04,perfOrigin:80000}),b=setup({begin:3.137,audioOrigin:3,latency:.18,perfOrigin:-30000});
 a.h.share(playback());b.h.share(playback());await a.click('play');a.advance(6);await b.click('play');b.advance(3);
 const left=sharedEvents(a),right=sharedEvents(b);assert.ok(right[0].absoluteStep>0);
 for(const e of right.filter(e=>e.serverAt<=106000)){
  const match=left.find(x=>x.absoluteStep===e.absoluteStep);assert.ok(match);assert.equal(match.serverAt,e.serverAt);
  assert.ok(Math.abs(100000+(match.time+.04)*1000-match.serverAt)<.01);
  assert.ok(Math.abs(100000+(e.time+3+.18)*1000-e.serverAt)<.01);
 }
});
test('actual audio scheduler: both devices apply tempo and notes at the scheduled boundary',async()=>{
 const a=setup(),b=setup({begin:.7,perfOrigin:50000});let p=playback();a.h.share(p);b.h.share(p);await a.click('play');await b.click('play');
 const changed=structuredClone(seed);changed.bpm=90;changed.pattern[0][0]=0;p=schedulePlayback(p,changed,101000,1);a.h.updatePlayback(p);b.h.updatePlayback(p);a.advance(8);b.advance(7.3);
 for(const client of [a,b]){const first=sharedEvents(client).find(e=>e.frameRevision===1);assert.equal(first.serverAt,p.frames.at(-1).at);assert.equal(first.hits[0],false);}
});
test('actual audio scheduler: stop then Play rejoins the current step',async()=>{
 const a=setup();a.h.share(playback());await a.click('play');a.advance(1);await a.click('play');
 assert.match(a.el('partner-status').textContent,/Press Play/);assert.match(a.el('listen-status').textContent,/Ready/);
 a.advance(2.12);await a.click('play');a.advance(.4);
 const tail=sharedEvents(a).at(-1);assert.ok(tail.absoluteStep>=13);assert.equal(tail.serverAt,100000+tail.absoluteStep*250);
});
test('actual audio scheduler: reconnect skips missed notes and rejoins without a burst',async()=>{
 const a=setup();a.h.share(playback());await a.click('play');a.advance(1);a.contactAge(5000);a.advance(3);const before=sharedEvents(a).length;a.contactAge(0);a.advance(.3);
 const resumed=sharedEvents(a).slice(before);assert.ok(resumed.length>0&&resumed.length<4);assert.ok(resumed[0].absoluteStep>=17);
});
test('actual audio scheduler: unacknowledged edits do not become audible early',async()=>{
 const a=setup();a.h.share(playback());await a.click('play');
 // Publish a desired UI state while the authoritative playback still contains the kick.
 const changed=structuredClone(seed);changed.pattern[0][0]=0;changed.bpm=60;
 a.h.share(initialPlayback(changed,100000,1));a.h.updatePlayback(playback());
 assert.equal(a.h.get().pattern[0][0],0);assert.equal(a.h.get().bpm,60);
 a.advance(3);const events=sharedEvents(a);assert.ok(events.filter(e=>e.absoluteStep%8===0).every(e=>e.hits[0]));
 assert.ok(events.length>8);assert.equal(events[1].serverAt-events[0].serverAt,250);
});
