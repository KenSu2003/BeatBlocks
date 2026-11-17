#pragma once
#include <cstring>
#include "kick_pcm.h"
#include "snare_pcm.h"
#include "generated/tom_pcm.h"
#include "generated/clap_pcm.h"
#include "generated/woodblock_pcm.h"
#include "generated/claves_pcm.h"

namespace beatblocks {
struct Sample { const int16_t* data; size_t length; };
enum class SoundSet : uint8_t { Drums, Percussion, Wood };
constexpr int kSoundSampleRate = 44100;
static_assert(KICK_SAMPLE_RATE == kSoundSampleRate && SNARE_SAMPLE_RATE == kSoundSampleRate,
              "All drum samples must use the I2S sample rate");
inline const char* soundSetId(SoundSet set) {
  switch(set) {
    case SoundSet::Percussion: return "percussion";
    case SoundSet::Wood: return "wood";
    default: return "drums";
  }
}
inline Sample sampleFor(SoundSet set, int row) {
  switch(set) {
    case SoundSet::Percussion: return row ? Sample{CLAP_PCM,CLAP_NUM_SAMPLES} : Sample{TOM_PCM,TOM_NUM_SAMPLES};
    case SoundSet::Wood: return row ? Sample{CLAVES_PCM,CLAVES_NUM_SAMPLES} : Sample{WOODBLOCK_PCM,WOODBLOCK_NUM_SAMPLES};
    default: return row ? Sample{SNARE_PCM,SNARE_NUM_SAMPLES} : Sample{KICK_PCM,KICK_NUM_SAMPLES};
  }
}
// Used only on the sequencer core. Each queued audio hit carries its own set.
struct SoundSetState {
  SoundSet active=SoundSet::Drums, pending=SoundSet::Drums;
  bool hasPending=false;
  bool queueCommand(const char* line) {
    SoundSet next;
    if(std::strcmp(line,"KIT:drums")==0) next=SoundSet::Drums;
    else if(std::strcmp(line,"KIT:percussion")==0) next=SoundSet::Percussion;
    else if(std::strcmp(line,"KIT:wood")==0) next=SoundSet::Wood;
    else return false;
    pending=next;hasPending=true;return true;
  }
  bool atStep(int step) {
    if(step!=0 || !hasPending) return false;
    active=pending;hasPending=false;return true;
  }
};
}
