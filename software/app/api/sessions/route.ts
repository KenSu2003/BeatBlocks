import {env} from 'cloudflare:workers';
import {handleSessionRequest} from '../../../lib/sessions.mjs';
const handler=(request:Request)=>handleSessionRequest(request,env.DB);
export {handler as GET,handler as POST,handler as OPTIONS};
