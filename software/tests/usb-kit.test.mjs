import test from 'node:test';
import assert from 'node:assert/strict';
import {KitControl} from '../../sound/reed_sim/kit-control.mjs';
test('USB set selection waits for active acknowledgement and ignores unrelated reed logs',()=>{
 const kit=new KitControl();assert.equal(kit.active,'drums');assert.equal(kit.supported,false);
 kit.receive('KIT:drums');assert.equal(kit.supported,true);
 assert.equal(kit.command('wood'),'KIT:wood\n');assert.equal(kit.active,'drums');
 kit.receive('OK queued KIT:wood');assert.equal(kit.pending,'wood');
 kit.receive('REED K[1,0,0,0,0,0,0,0]');assert.equal(kit.active,'drums');
 kit.receive('OK live KIT:wood');assert.equal(kit.active,'wood');assert.equal(kit.pending,null);
});
test('USB invalid values and disconnect preserve active selection and clear pending',()=>{
 const kit=new KitControl();kit.receive('KIT:percussion');
 assert.throws(()=>kit.command('unknown'));kit.receive('OK live KIT:unknown');assert.equal(kit.active,'percussion');
 kit.command('wood');kit.disconnected();assert.equal(kit.pending,null);assert.equal(kit.active,'percussion');assert.equal(kit.supported,false);
});
