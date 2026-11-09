import test from 'node:test';
import assert from 'node:assert/strict';
import {DatabaseSync} from 'node:sqlite';
import {readFileSync} from 'node:fs';
import {handleSessionRequest} from '../lib/sessions.mjs';

function fixture(){
 const sql=new DatabaseSync(':memory:');
 sql.exec('CREATE TABLE rooms(id TEXT PRIMARY KEY,state TEXT NOT NULL,revision INTEGER NOT NULL,updated_at INTEGER NOT NULL);CREATE TABLE participants(room_id TEXT NOT NULL,client_id TEXT NOT NULL,seen_at INTEGER NOT NULL,PRIMARY KEY(room_id,client_id));CREATE TABLE limits(key TEXT PRIMARY KEY,count INTEGER NOT NULL);');
 const db={withSession(){return this;},prepare(query){let args=[];return {bind(...v){args=v;return this;},async first(){return sql.prepare(query).get(...args)||null;},async all(){return {results:sql.prepare(query).all(...args)};},async run(){return {meta:sql.prepare(query).run(...args)};}};}};
 const request=async(path='',method='GET',body,origin='https://hitianji.github.io')=>{
  const r=await handleSessionRequest(new Request('https://beatblocks-playground.homerclare.chatgpt.site/api/sessions'+path,{method,headers:{origin,'Content-Type':'application/json','CF-Connecting-IP':'127.0.0.1'},body:body===undefined?undefined:JSON.stringify(body)}),db);
  return {status:r.status,data:r.status===204?null:await r.json(),headers:r.headers};
 };
 return {request,sql};
}
const seed={pattern:[[1,0,0,0,1,0,0,0],[0,0,1,0,0,0,1,0]],partner:[0,0,0,0,0,0,0,0],bpm:96};
const cid='a'.repeat(32),other='b'.repeat(32);
const edit=(seq,changes,clientId=cid)=>({type:'edit',clientId,seq,changes});

test('all clients receive the same playback epoch and future change timestamp',async()=>{
 const {request}=fixture();const {data:a}=await request('','POST',seed);
 assert.equal(a.playback.frames.length,1);const epoch=a.playback.frames[0].at;
 assert.equal((await request('/'+a.id)).data.playback.frames[0].at,epoch);
 const before=Date.now();const changed=await request('/'+a.id,'POST',edit(1,[{bpm:120},{row:2,step:0,value:1}]));
 assert.ok(changed.data.playback.frames.at(-1).at>=before+1800);
 assert.equal(changed.data.playback.frames.at(-1).step%8,0);
 assert.deepEqual((await request('/'+a.id)).data.playback,changed.data.playback);
 assert.equal(changed.data.playback.frames[0].bpm,96);
 assert.equal(changed.data.playback.frames.at(-1).bpm,120);
});
test('clock sampling works without reading a room or database',async()=>{
 const r=await handleSessionRequest(new Request('https://beatblocks-playground.homerclare.chatgpt.site/api/sessions?clock=1'),null);
 assert.equal(r.status,200);const data=await r.json();assert.ok(data.serverTime>=data.serverReceived);
});
test('existing invitation links gain a stable clock without losing their beat',async()=>{
 const {request,sql}=fixture();const id='d'.repeat(64);
 sql.prepare('INSERT INTO rooms VALUES(?,?,?,?)').run(id,JSON.stringify({...seed,seen:{}}),7,100000);
 const first=await request('/'+id);assert.equal(first.data.playback.frames[0].at,100000);
 await request('/'+id,'POST',edit(1,[{row:0,step:1,value:1}]));
 const after=await request('/'+id);assert.equal(after.data.playback.frames[0].at,100000);assert.equal(after.data.state.pattern[0][1],1);
});

test('create, join from another client, and recover saved state',async()=>{
 const {request}=fixture();const a=await request('','POST',seed);
 assert.equal(a.status,201);assert.match(a.data.id,/^[a-f0-9]{64}$/);
 const b=await request('/'+a.data.id);assert.deepEqual(b.data.state.pattern,seed.pattern);
 assert.equal((await request('/'+a.data.id,'POST',{type:'presence',clientId:cid})).data.connected,1);
 assert.equal((await request('/'+a.data.id,'POST',{type:'presence',clientId:other})).data.connected,2);
});
test('simultaneous edits to different cells both survive',async()=>{
 const {request}=fixture();const {data:{id}}=await request('','POST',seed);
 await Promise.all([request('/'+id,'POST',edit(1,[{row:0,step:1,value:1}])),request('/'+id,'POST',edit(1,[{row:1,step:3,value:1}],other))]);
 const {data}=await request('/'+id);assert.equal(data.state.pattern[0][1],1);assert.equal(data.state.pattern[1][3],1);assert.equal(data.revision,2);
});
test('retrying an acknowledged edit never overwrites a later edit',async()=>{
 const {request}=fixture();const {data:{id}}=await request('','POST',seed);
 const one=edit(1,[{row:0,step:1,value:1}]);
 await request('/'+id,'POST',one);await request('/'+id,'POST',edit(1,[{row:0,step:1,value:0}],other));
 const {data}=await request('/'+id,'POST',one);assert.equal(data.state.pattern[0][1],0);assert.equal(data.revision,2);
});
test('hi-hat and tempo edits persist, and room data stays isolated',async()=>{
 const {request}=fixture();const {data:{id}}=await request('','POST',seed);const second=await request('','POST',seed);
 await request('/'+id,'POST',edit(1,[{row:2,step:7,value:1},{bpm:125}]));
 const a=await request('/'+id);assert.equal(a.data.state.partner[7],1);assert.equal(a.data.state.bpm,125);
 assert.deepEqual((await request('/'+second.data.id)).data.state,seed);
});
test('invalid payloads and unknown rooms fail without altering the beat',async()=>{
 const {request}=fixture();const {data:{id}}=await request('','POST',seed);
 for(const changes of [[{row:3,step:0,value:1}],[{bpm:999}],[{row:0,step:8,value:0}],[{row:0,step:1,value:'1'}]])assert.equal((await request('/'+id,'POST',edit(1,changes))).status,400);
 assert.equal((await request('/'+'0'.repeat(64))).status,404);
 assert.equal((await request('/'+id)).data.revision,0);
});
test('CORS permits the demo and rejects unrelated origins',async()=>{
 const {request}=fixture();const r=await request('','OPTIONS');assert.equal(r.status,204);assert.equal(r.headers.get('Access-Control-Allow-Origin'),'https://hitianji.github.io');
 assert.equal((await request('','POST',seed,'https://unrelated.example')).status,403);
});
