import {isSoundSet} from './sound-catalog.mjs';
export class KitControl {
 active='drums';pending=null;supported=false;
 command(id){
  if(!isSoundSet(id))throw new Error('Unknown sound set.');
  this.pending=id;return 'KIT:'+id+'\n';
 }
 receive(line){
  const match=line.match(/^(KIT:|OK live KIT:|OK queued KIT:)([a-z]+)$/);
  if(!match||!isSoundSet(match[2]))return false;
  this.supported=true;
  if(match[1]==='OK queued KIT:')this.pending=match[2];
  else{this.active=match[2];if(this.pending===this.active||match[1]==='KIT:')this.pending=null;}
  return true;
 }
 disconnected(){this.supported=false;this.pending=null;}
}
