#pragma once
#include "SoundBank.h"

namespace beatblocks {
// Eight voices cover overlapping two-row hits at 170 BPM with the supplied samples.
// No allocation, sample conversion, or serial work in the audio loop.
class SampleMixer {
  struct Voice { Sample sample{nullptr,0}; size_t position=0; };
  Voice voices[8];
public:
  bool trigger(Sample sample) {
    for(auto& voice:voices) {
      if(voice.position>=voice.sample.length) {
        voice={sample,0};return true;
      }
    }
    return false;
  }
  void render(int16_t* output,size_t frames) {
    for(size_t i=0;i<frames;++i) {
      int32_t sum=0;
      for(auto& voice:voices) {
        if(voice.position<voice.sample.length)
          sum+=static_cast<int16_t>(pgm_read_word(voice.sample.data+voice.position++));
      }
      output[i]=static_cast<int16_t>(sum>32767 ? 32767 : sum< -32768 ? -32768 : sum);
    }
  }
};
}
