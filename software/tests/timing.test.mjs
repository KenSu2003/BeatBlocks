import test from 'node:test';
import assert from 'node:assert/strict';
import {initialPlayback,schedulePlayback,positionAt,nextTick,ServerClock,outputMapping} from '../lib/timing.mjs';
const beat=(bpm=120)=>({pattern:[[1,0,0,0,1,0,0,0],[0,0,1,0,0,0,1,0]],partner:Array(8).fill(0),bpm});
test('different start times join the same absolute steps',()=>{
 const p=initialPlayback(beat(),100000,0);
 const a=nextTick(p,100040),b=nextTick(p,103120);
 assert.equal(a.step,1);assert.equal(a.at,100250);assert.equal(b.step,13);assert.equal(b.at,103250);
 assert.equal(nextTick(p,103120,a.step).at,b.at);
 assert.equal(positionAt(p,103250).step,13);
});
test('tempo and notes change together on a future shared bar',()=>{
 const p=initialPlayback(beat(),100000,0);const changed=beat(60);changed.pattern[0][1]=1;
 const q=schedulePlayback(p,changed,100500,1);
 assert.equal(q.frames[1].at,104000);assert.equal(q.frames[1].step,16);
 assert.equal(positionAt(q,103999).frame.bpm,120);assert.equal(positionAt(q,104000).frame.bpm,60);
 assert.equal(nextTick(q,104001).at,104500);assert.equal(nextTick(q,104001).frame.pattern[0][1],1);
});
test('edits near an announced change preserve it and schedule the following bar',()=>{
 let p=schedulePlayback(initialPlayback(beat(),100000,0),beat(60),100500,1);
 p=schedulePlayback(p,beat(90),103000,2);
 assert.equal(positionAt(p,104001).frame.bpm,60);assert.equal(p.frames.at(-1).at,108000);assert.equal(p.frames.at(-1).step,24);
});
test('early edits can combine, preserving all future boundary positions',()=>{
 let p=schedulePlayback(initialPlayback(beat(),100000,0),beat(60),100100,1);
 p=schedulePlayback(p,beat(140),100150,2);assert.equal(p.frames.length,2);assert.equal(p.frames[1].at,102000);assert.equal(p.frames[1].bpm,140);
});
test('server time estimation removes server processing delay and ignores device wall clocks',()=>{
 let mono=1000;const c=new ServerClock(()=>mono);
 c.observe({serverReceived:500020,serverTime:500120},1000,1140);
 mono=1140;assert.equal(c.now(),500140);assert.equal(c.errorMs,20);
 c.observe({serverReceived:500160,serverTime:500160},1160,1170);mono=1170;
 assert.ok(Math.abs(c.now()-500165)<1);
});
test('low-latency clock samples outweigh queued responses and expire',()=>{
 let mono=1000;const c=new ServerClock(()=>mono);c.observe({serverReceived:100010,serverTime:100010},1000,1020);
 c.observe({serverReceived:100110,serverTime:100110},1030,1450);mono=1450;assert.equal(c.now(),100450);
 mono=62000;c.observe({serverReceived:161515,serverTime:161515},62000,62010);mono=62010;assert.equal(c.now(),161520);
});
test('speaker output mapping compensates for different device latencies',()=>{
 const a=outputMapping({currentTime:10,getOutputTimestamp:()=>({contextTime:9.96,performanceTime:1000})},1000);
 const b=outputMapping({currentTime:4,getOutputTimestamp:()=>({contextTime:3.8,performanceTime:1000})},1000);
 const targetPerf=1500,atA=a.audioBase+(targetPerf-a.perfBase)/1000,atB=b.audioBase+(targetPerf-b.perfBase)/1000;
 assert.equal(atA,10.46);assert.equal(atB,4.3);
 assert.ok(Math.abs((atA-10+.04)-(atB-4+.2))<1e-10);
});
test('fallback output mapping uses reported base and output latency',()=>{
 assert.deepEqual(outputMapping({currentTime:2,baseLatency:.01,outputLatency:.03},1000),{audioBase:2,perfBase:1040});
});
test('late scheduler wake-ups skip missed notes instead of bursting or restarting',()=>{
 const p=initialPlayback(beat(),100000,0);const resumed=nextTick(p,111040,4);assert.equal(resumed.step,45);assert.equal(resumed.at,111250);
});
