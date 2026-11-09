// Server milliseconds define musical time. A step is an eighth note; a bar is eight steps.
export function initialPlayback(state,at,revision){return {frames:[{at,step:0,revision,bpm:state.bpm,pattern:state.pattern.map(r=>r.slice()),partner:state.partner.slice()}]};}
export function positionAt(playback,time){
 const frames=playback.frames;let index=0;
 while(index+1<frames.length&&frames[index+1].at<=time+.001)index++;
 const frame=frames[index],duration=30000/frame.bpm;
 const step=frame.step+Math.max(0,Math.floor((time-frame.at+.001)/duration));
 return {frame,index,step,at:frame.at+(step-frame.step)*duration};
}
export function nextTick(playback,time,afterStep=-1){
 const pos=positionAt(playback,time),frames=playback.frames;
 let i=pos.index;
 while(true){
  const frame=frames[i],duration=30000/frame.bpm;
  const step=Math.max(frame.step,afterStep+1,frame.step+Math.ceil((time-frame.at-.001)/duration));
  const at=frame.at+(step-frame.step)*duration;
  if(i+1<frames.length&&at>=frames[i+1].at-.001){i++;continue;}
  return {frame,step,at};
 }
}
export function schedulePlayback(playback,state,now,revision){
 const earliest=now+1800;
 let tick=nextTick(playback,earliest);
 while(tick.step%8!==0)tick=nextTick(playback,tick.at+.01,tick.step);
 const frame={at:tick.at,step:tick.step,revision,bpm:state.bpm,pattern:state.pattern.map(r=>r.slice()),partner:state.partner.slice()};
 const retained=playback.frames.filter(f=>f.at<tick.at-.001);
 while(retained.length>1&&retained[1].at<now-10000)retained.shift();
 return {frames:[...retained,frame]};
}
export class ServerClock {
 constructor(now=()=>performance.now()){this.monotonic=now;this.samples=[];this.offset=null;this.errorMs=Infinity;this.lastSample=-Infinity;}
 observe(data,sent,received){
  const {serverReceived,serverTime}=data;
  if(!Number.isFinite(serverReceived)||!Number.isFinite(serverTime)||serverTime<serverReceived||received<sent)return;
  const rtt=received-sent-(serverTime-serverReceived);
  if(rtt< -5||rtt>2000)return;
  this.samples=this.samples.filter(s=>received-s.received<60000);
  this.samples.push({received,rtt:Math.max(0,rtt),offset:serverTime+Math.max(0,rtt)/2-received});
  if(this.samples.length>80)this.samples.shift();
  // Minimum round-trip delay minimizes queueing bias; refresh the sample set every minute.
  const best=this.samples.reduce((a,b)=>b.rtt<=a.rtt?b:a);
  this.offset=best.offset;this.errorMs=best.rtt/2;this.lastSample=received;
 }
 get ready(){return this.offset!==null;}
 now(){return this.ready?this.monotonic()+this.offset:NaN;}
 toServer(performanceTime){return performanceTime+this.offset;}
 toPerformance(serverTime){return serverTime-this.offset;}
}
export function outputMapping(context,perfNow){
 const ts=context.getOutputTimestamp?.();
 if(ts&&ts.contextTime>0&&ts.performanceTime>0&&Math.abs(perfNow-ts.performanceTime)<1000)return {audioBase:ts.contextTime,perfBase:ts.performanceTime};
 return {audioBase:context.currentTime,perfBase:perfNow+1000*((context.baseLatency||0)+(context.outputLatency||0))};
}
