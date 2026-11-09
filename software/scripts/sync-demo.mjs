import {readFileSync,writeFileSync} from 'node:fs';
const path=new URL('../public/index.html',import.meta.url);
const timing=readFileSync(new URL('../lib/timing.mjs',import.meta.url),'utf8').replaceAll('export ','');
const client=timing+'\n'+readFileSync(new URL('../lib/session-client.mjs',import.meta.url),'utf8').replace(/^import .*?;\n/,'').replace('export class','class');
const html=readFileSync(path,'utf8').replace(/\/\/ shared-client:start[\s\S]*?\/\/ shared-client:end/,'// shared-client:start\n'+client+'// shared-client:end');
writeFileSync(path,html);
