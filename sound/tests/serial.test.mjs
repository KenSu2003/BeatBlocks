import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
const source=readFileSync(new URL('../reed_bridge/reed_bridge.ino',import.meta.url),'utf8');
function functionSource(name){
 const start=source.indexOf('static void '+name+'(')>=0?source.indexOf('static void '+name+'('):source.indexOf('void '+name+'(');
 if(start<0)return '';
 let cursor=source.indexOf('{',start),depth=1;
 for(cursor++;depth;cursor++){if(source[cursor]==='{')depth++;if(source[cursor]==='}')depth--;}
 return source.slice(start,cursor);
}
function run(body){
 const dir=mkdtempSync(join(tmpdir(),'beatblocks-serial-'));
 try{
  const cpp=join(dir,'test.cpp'),binary=join(dir,'test');writeFileSync(cpp,body);
  const built=spawnSync('c++',['-std=c++17','-I',fileURLToPath(new URL('./stubs/',import.meta.url)),'-I',fileURLToPath(new URL('../reed_bridge/',import.meta.url)),cpp,'-o',binary],{encoding:'utf8'});
  assert.equal(built.status,0,built.stderr);
  const result=spawnSync(binary,[],{encoding:'utf8'});assert.equal(result.status,0,result.stderr||result.stdout);
 }finally{rmSync(dir,{recursive:true,force:true});}
}
test('actual serial framing discards the complete overlong or NUL-containing line',()=>{
 const globals=source.slice(source.indexOf('static char lineBuf'),source.indexOf('static int readBpmFromPot'));
 run(`#include <string>\n#include <vector>\n#include <cassert>\n#include <cstdio>\nusing namespace std;
 struct SerialFake { string input;size_t at=0;int available(){return at<input.size();}int read(){return input[at++];}void println(const char*){} } Serial;
 vector<string> commands;void handleLine(char* line){commands.emplace_back(line);}void serialLog(const char*){}
 ${globals}\n${functionSource('pollSerial')}
 int main(){
  Serial.input=string(96,'x')+"KIT:wood\\n";pollSerial();assert(commands.empty());
  Serial.input=string("KIT:wood\\0junk\\n",14);Serial.at=0;pollSerial();assert(commands.empty());
  Serial.input="KIT:drums\\n";Serial.at=0;pollSerial();assert(commands.size()==1&&commands[0]=="KIT:drums");
 }
 `);
});
test('actual sequencer retries its active sound-set acknowledgement after TX congestion',()=>{
 run(`#include "SoundBank.h"\n#include <cassert>\n#include <cstdarg>\n#include <cstdio>\n#include <string>
 struct SerialFake{int free=0;std::string output;int availableForWrite(){return free;}void printf(const char* format,...){char text[128];va_list a;va_start(a,format);vsnprintf(text,sizeof(text),format,a);va_end(a);output+=text;}}Serial;
 beatblocks::SoundSetState soundSetState;${source.includes('kitReportPending')?'bool kitReportPending=false;':''}
 constexpr int kNumSteps=8;int8_t uiKick[8]{},uiSnare[8]{},reedKick[8]{},reedSnare[8]{};bool audioReady=false;void* hitQueue=nullptr;
 struct AudioHit{bool top;bool bottom;beatblocks::SoundSet soundSet;};void xQueueOverwrite(void*,const AudioHit*){}
 void setPlayhead(int){}void pollSerial(){}void applyPendingIfAny(){}void scanReeds(){}void reportReedsIfChanged(){}int currentBpm(){return 96;}int stepDurationMs(int){return 1;}void waitStepMs(int){Serial.free=128;}
 ${functionSource('reportActiveSoundSet')}
 ${functionSource('loop')}
 int main(){soundSetState.queueCommand("KIT:wood");loop();assert(soundSetState.active==beatblocks::SoundSet::Wood);assert(Serial.output.find("OK live KIT:wood")!=std::string::npos);}
 `);
});
