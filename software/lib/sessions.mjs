import {initialPlayback,schedulePlayback} from './timing.mjs';
const ROW=v=>Array.isArray(v)&&v.length===8&&v.every(x=>x===0||x===1);
const TEMPO=v=>Number.isInteger(v)&&v>=50&&v<=170;
const TOKEN=/^[a-f0-9]{64}$/;
const CLIENT=/^[a-f0-9]{32}$/;
const validState=s=>s&&Array.isArray(s.pattern)&&s.pattern.length===2&&s.pattern.every(ROW)&&ROW(s.partner)&&TEMPO(s.bpm);
const validChange=c=>c&&typeof c==='object'&&(Object.keys(c).length===1&&TEMPO(c.bpm)||Object.keys(c).length===3&&Number.isInteger(c.row)&&c.row>=0&&c.row<=2&&Number.isInteger(c.step)&&c.step>=0&&c.step<8&&(c.value===0||c.value===1));
const publicState=s=>({pattern:s.pattern,partner:s.partner,bpm:s.bpm});
const fail=(message,status=400)=>Object.assign(new Error(message),{status});
async function bodyOf(request){
 if(!request.headers.get('Content-Type')?.startsWith('application/json'))throw fail('Send JSON.');
 const reader=request.body?.getReader();if(!reader)throw fail('Missing beat.');let bytes=0,chunks=[];
 while(true){const {value,done}=await reader.read();if(done)break;bytes+=value.byteLength;if(bytes>10000){await reader.cancel();throw fail('Request too large.',413);}chunks.push(value);}
 const combined=new Uint8Array(bytes);let at=0;for(const c of chunks){combined.set(c,at);at+=c.length;}
 try{return JSON.parse(new TextDecoder().decode(combined));}catch{throw fail('Invalid JSON.');}
}
async function rate(db,key,max){
 const r=await db.prepare('INSERT INTO limits(key,count) VALUES(?,1) ON CONFLICT(key) DO UPDATE SET count=count+1 RETURNING count').bind(key).first();
 if(r.count>max)throw fail('Too many changes. Try again in a minute.',429);
}
async function responseState(db,room){
 const state=JSON.parse(room.state);
 const count=await db.prepare('SELECT COUNT(*) AS count FROM participants WHERE room_id=? AND seen_at>?').bind(room.id,Date.now()-35000).first();
 return {id:room.id,revision:room.revision,state:publicState(state),playback:state.playback||initialPlayback(state,room.updated_at,room.revision),connected:count.count};
}
export async function handleSessionRequest(request,binding){
 const serverReceived=Date.now();
 const origin=request.headers.get('Origin'),url=new URL(request.url);
 const allowed=!origin||origin===url.origin||origin==='https://hitianji.github.io'||/^http:\/\/(localhost|127\.0\.0\.1):\d+$/.test(origin);
 const headers={'Content-Type':'application/json','Cache-Control':'no-store','Vary':'Origin','Access-Control-Allow-Methods':'GET, POST, OPTIONS','Access-Control-Allow-Headers':'Content-Type','Access-Control-Max-Age':'600'};
 if(origin&&allowed)headers['Access-Control-Allow-Origin']=origin;
 const json=(data,status=200)=>new Response(JSON.stringify({...data,serverReceived,serverTime:Date.now()}),{status,headers});
 if(!allowed)return json({error:'Origin not allowed.'},403);
 if(request.method==='OPTIONS')return new Response(null,{status:204,headers});
 try{
  if(request.method==='GET'&&url.pathname==='/api/sessions'&&url.searchParams.get('clock')==='1')return json({clock:true});
  if(!binding)throw fail('Session service is starting. Try again shortly.',503);
  const db=binding.withSession('first-primary');
  const id=url.pathname.replace(/^\/api\/sessions\/?/,'');
  if(!id&&request.method==='POST'){
   const input=await bodyOf(request);if(!validState(input))throw fail('Invalid beat.');
   const ip=request.headers.get('CF-Connecting-IP')||'unknown';
   const hash=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',new TextEncoder().encode(ip)))).map(x=>x.toString(16).padStart(2,'0')).join('');
   await rate(db,'create:'+hash+':'+Math.floor(Date.now()/86400000),50);
   const token=Array.from(crypto.getRandomValues(new Uint8Array(32)),x=>x.toString(16).padStart(2,'0')).join('');
   const state={...publicState(input),seen:{},playback:initialPlayback(input,Date.now(),0)};
   const room={id:token,state:JSON.stringify(state),revision:0,updated_at:Date.now()};
   await db.prepare('INSERT INTO rooms(id,state,revision,updated_at) VALUES(?,?,?,?)').bind(token,room.state,0,room.updated_at).run();
   return json(await responseState(db,room),201);
  }
  if(!TOKEN.test(id))throw fail('Session not found. Check the invitation link.',404);
  let room=await db.prepare('SELECT * FROM rooms WHERE id=?').bind(id).first();
  if(!room)throw fail('Session not found. Check the invitation link.',404);
  if(request.method==='GET')return json(await responseState(db,room));
  if(request.method!=='POST')throw fail('Method not allowed.',405);
  const input=await bodyOf(request);
  if(!input||!CLIENT.test(input.clientId))throw fail('Invalid connection. Reload the page.');
  if(input.type==='presence'){
   await rate(db,'presence:'+id+':'+Math.floor(Date.now()/60000),240);
   await db.prepare('INSERT INTO participants(room_id,client_id,seen_at) VALUES(?,?,?) ON CONFLICT(room_id,client_id) DO UPDATE SET seen_at=excluded.seen_at').bind(id,input.clientId,Date.now()).run();
   return json(await responseState(db,room));
  }
  if(input.type!=='edit'||!Number.isSafeInteger(input.seq)||input.seq<1||!Array.isArray(input.changes)||!input.changes.length||input.changes.length>25||!input.changes.every(validChange))throw fail('Invalid change.');
  await rate(db,'edit:'+id+':'+Math.floor(Date.now()/60000),600);
  for(let attempt=0;attempt<8;attempt++){
   const state=JSON.parse(room.state);
   if((state.seen[input.clientId]||0)>=input.seq)return json(await responseState(db,room));
   if(Object.keys(state.seen).length>=256&&!state.seen[input.clientId])throw fail('This session is full. Create a new session.',409);
   const previousPlayback=state.playback||initialPlayback(state,room.updated_at,room.revision);
   for(const change of input.changes){if('bpm' in change)state.bpm=change.bpm;else (change.row===2?state.partner:state.pattern[change.row])[change.step]=change.value;}
   state.seen[input.clientId]=input.seq;
   state.playback=schedulePlayback(previousPlayback,state,Date.now(),room.revision+1);
   const next={id,state:JSON.stringify(state),revision:room.revision+1,updated_at:Date.now()};
   const result=await db.prepare('UPDATE rooms SET state=?,revision=?,updated_at=? WHERE id=? AND revision=?').bind(next.state,next.revision,next.updated_at,id,room.revision).run();
   if(Number(result.meta.changes)===1)return json(await responseState(db,next));
   room=await db.prepare('SELECT * FROM rooms WHERE id=?').bind(id).first();
  }
  throw fail('Session busy. Retrying shortly.',503);
 }catch(error){return json({error:error.status?error.message:'Connection interrupted. Retrying shortly.'},error.status||503);}
}
