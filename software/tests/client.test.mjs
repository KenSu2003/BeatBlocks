import test from 'node:test';
import assert from 'node:assert/strict';
import {SharedSessionClient} from '../lib/session-client.mjs';
const state=()=>({pattern:[Array(8).fill(0),Array(8).fill(0)],partner:Array(8).fill(0),bpm:96});
const response=(revision,s=state())=>({revision,state:s,connected:2});
const pause=()=>new Promise(r=>setImmediate(r));
test('native browser fetch retains its Window receiver',async()=>{
 const original=globalThis.fetch;
 globalThis.fetch=async function(){assert.equal(this,globalThis);return {ok:true,json:async()=>response(0)};};
 try{const c=new SharedSessionClient({roomId:'x',onState(){},onStatus(){}});assert.equal((await c.request()).revision,0);}finally{globalThis.fetch=original;}
});
test('older poll responses cannot undo newer edits',()=>{
 const c=new SharedSessionClient({roomId:'x',onState(){},onStatus(){}});let s=state();s.pattern[0][2]=1;c.accept(response(2,s));c.accept(response(1));assert.equal(c.snapshot().pattern[0][2],1);
});
test('queued edits overlay remote changes without overwriting unrelated notes',async()=>{
 let reject;const c=new SharedSessionClient({roomId:'x',clientId:'a',fetch:()=>new Promise((_,r)=>reject=r),onState(){},onStatus(){}});c.accept(response(0));
 c.edit([{row:0,step:1,value:1}]);let remote=state();remote.pattern[1][3]=1;c.accept(response(1,remote));
 assert.equal(c.snapshot().pattern[0][1],1);assert.equal(c.snapshot().pattern[1][3],1);
 reject(new Error('offline'));await pause();assert.equal(c.queue.length,1);c.close();
});
test('a failed request retries with the same sequence and drains in order',async()=>{
 let attempts=[],fail=true;let s=state(),rev=0;
 const c=new SharedSessionClient({roomId:'x',clientId:'a',onState(){},onStatus(){},fetch:async(url,options)=>{
  const op=JSON.parse(options.body);attempts.push(op.seq);if(fail)throw new Error('offline');
  for(const p of op.changes)s.pattern[p.row][p.step]=p.value;
  return {ok:true,json:async()=>response(++rev,structuredClone(s))};
 }});c.accept(response(0));c.edit([{row:0,step:0,value:1}]);await pause();c.edit([{row:1,step:2,value:1}]);await pause();fail=false;await c.flush();
 assert.deepEqual(attempts,[1,1,1,2]);assert.equal(c.queue.length,0);assert.equal(c.snapshot().pattern[1][2],1);c.close();
});
test('an acknowledgement older than a poll does not roll the board back',async()=>{
 let resolve;const c=new SharedSessionClient({roomId:'x',clientId:'a',onState(){},onStatus(){},fetch:()=>new Promise(r=>resolve=r)});c.accept(response(0));c.edit([{row:0,step:0,value:1}]);
 let newest=state();newest.pattern[0][0]=0;newest.pattern[1][1]=1;c.accept(response(3,newest));
 resolve({ok:true,json:async()=>response(1)});await pause();assert.equal(c.snapshot().pattern[1][1],1);assert.equal(c.snapshot().pattern[0][0],0);c.close();
});
