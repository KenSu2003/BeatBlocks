export const DEFAULT_SOUND_SET = 'drums';
export const SOUND_SETS = Object.freeze({
 drums: {label:'Drums', instruments:['kick','snare'], names:['Kick','Snare'], hints:['Low thump','Sharp snap']},
 percussion: {label:'Percussion', instruments:['tom','clap'], names:['Tom','Clap'], hints:['Round drum','Hand clap']},
 wood: {label:'Wood', instruments:['woodblock','claves'], names:['Woodblock','Claves'], hints:['Hollow knock','Bright click']},
});
export function isSoundSet(id){return typeof id==='string'&&Object.hasOwn(SOUND_SETS,id);}
export function soundSetOrDefault(id){return isSoundSet(id)?id:DEFAULT_SOUND_SET;}
