import {ServerClock} from './timing.mjs';
export class SharedSessionClient {
 constructor(options){
  this.baseUrl=options.baseUrl||'';this.roomId=options.roomId;
  this.clientId=options.clientId||crypto.randomUUID().replaceAll('-','');
  this.fetch=options.fetch||fetch.bind(globalThis);this.onState=options.onState;this.onStatus=options.onStatus;
  this.monotonic=options.monotonic||(()=>performance.now());this.clock=new ServerClock(this.monotonic);this.playback=null;this.lastContact=-Infinity;
  this.revision=-1;this.canonical=null;this.queue=[];this.seq=0;this.connected=0;
  this.closed=false;this.sending=false;this.polling=false;this.heartbeat=0;this.timer=null;this.online=false;
 }
 snapshot(){
  if(!this.canonical)return null;
  const s=JSON.parse(JSON.stringify(this.canonical));
  for(const op of this.queue)for(const c of op.changes){if('bpm' in c)s.bpm=c.bpm;else (c.row===2?s.partner:s.pattern[c.row])[c.step]=c.value;}
  return s;
 }
 emit(){if(!this.closed&&this.canonical)this.onState(this.snapshot());}
 status(text){if(!this.closed)this.onStatus(text);}
 connectedStatus(){this.status('Connected · '+Math.max(1,this.connected)+' here'+(this.queue.length?' · saving…':' · saved'));}
 accept(data){
  if(this.closed)return;
  this.connected=data.connected;
  if(data.revision>=this.revision){this.revision=data.revision;this.canonical=data.state;this.playback=data.playback||null;}
  this.lastContact=this.monotonic();
  this.online=true;this.emit();this.connectedStatus();
 }
 async request(body,clockOnly=false){
  const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),8000);
  try{
   const sent=this.monotonic();
   const res=await this.fetch(this.baseUrl+'/api/sessions'+(clockOnly?'?clock=1':'/'+this.roomId),{method:body?'POST':'GET',headers:body?{'Content-Type':'application/json'}:{},body:body?JSON.stringify(body):undefined,signal:controller.signal,cache:'no-store',credentials:'omit',referrerPolicy:'no-referrer'});
   const data=await res.json();const received=this.monotonic();if(!res.ok)throw Object.assign(new Error(data.error||'Could not connect.'),{status:res.status});this.clock.observe(data,sent,received);return data;
  }finally{clearTimeout(timeout);}
 }
 failed(error){
  this.online=false;
  if([400,403,404,409,413].includes(error.status)){this.status(error.message);this.fatal=true;return;}
  this.status('Reconnecting'+(this.queue.length?' · '+this.queue.length+(this.queue.length===1?' change waiting':' changes waiting'):'')+'…');
 }
 edit(changes){
  if(!this.canonical||this.closed||this.fatal)return false;
  if(this.queue.length>=256){this.status('Reconnect to send your waiting changes.');return false;}
  this.queue.push({type:'edit',clientId:this.clientId,seq:++this.seq,changes:JSON.parse(JSON.stringify(changes))});
  this.emit();this.online?this.connectedStatus():this.status('Reconnecting · '+this.queue.length+(this.queue.length===1?' change waiting…':' changes waiting…'));this.flush();return true;
 }
 async flush(){
  if(this.sending||this.closed||this.fatal)return;
  this.sending=true;
  try{while(this.queue.length&&!this.closed){const op=this.queue[0],data=await this.request(op);if(this.closed)break;this.queue.shift();this.accept(data);}}
  catch(error){this.failed(error);}finally{this.sending=false;}
 }
 async poll(){
  if(this.closed||this.polling||this.fatal)return;
  this.polling=true;
  try{const heartbeat=Date.now()-this.heartbeat>10000;const data=await this.request(heartbeat?{type:'presence',clientId:this.clientId}:null);if(heartbeat)this.heartbeat=Date.now();this.accept(data);await this.flush();}
  catch(error){this.failed(error);}finally{
   this.polling=false;
   if(!this.closed&&!this.fatal)this.timer=setTimeout(()=>this.poll(),this.online?600:2000);
  }
 }
 async syncClock(count=5){for(let i=0;i<count&&!this.closed;i++)await this.request(null,true);}
 start(){this.status('Joining session…');return Promise.all([this.poll(),this.syncClock().catch(()=>{})]);}
 close(){this.closed=true;clearTimeout(this.timer);}
}
