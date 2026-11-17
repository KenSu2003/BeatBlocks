import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {SOUND_PCM,SOUND_SAMPLE_RATE} from '../lib/sound-samples.mjs';
import {SOUND_SETS} from '../lib/sound-catalog.mjs';
test('every sound set has bounded, non-silent PCM matching the firmware assets',()=>{
 assert.equal(SOUND_SAMPLE_RATE,44100);
 for(const id of new Set(Object.values(SOUND_SETS).flatMap(s=>s.instruments))){
  const bytes=Buffer.from(SOUND_PCM[id],'base64');assert.equal(bytes.length%2,0);
  const prefix=['kick','snare'].includes(id)?'':'generated/';
  const header=readFileSync(new URL('../../sound/reed_bridge/'+prefix+id+'_pcm.h',import.meta.url),'utf8');
  const expected=header.match(/PROGMEM = \{([\s\S]*?)\};/)[1].match(/-?\d+/g).map(Number);
  assert.equal(bytes.length,expected.length*2,id);
  let energy=0;for(let i=0;i<expected.length;i++){assert.equal(bytes.readInt16LE(i*2),expected[i],id);energy+=Math.abs(expected[i]);}
  assert.ok(energy>1000,id);assert.ok(expected.length<=44100*.4,id);
 }
});
