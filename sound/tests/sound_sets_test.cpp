#include <cassert>
#include <cstdio>
#include <vector>
#include "../reed_bridge/SoundBank.h"
#include "../reed_bridge/SampleMixer.h"

int main() {
  using namespace beatblocks;
  SoundSetState state;
  assert(state.active == SoundSet::Drums);
  assert(state.queueCommand("KIT:wood"));
  assert(!state.queueCommand("KIT:unknown"));
  assert(!state.queueCommand("KIT:woodjunk"));
  assert(!state.atStep(7));
  assert(state.active == SoundSet::Drums);
  assert(state.atStep(0));
  assert(state.active == SoundSet::Wood);
  assert(state.queueCommand("KIT:drums"));
  assert(state.atStep(0));
  assert(sampleFor(SoundSet::Drums,0).data == KICK_PCM);
  assert(sampleFor(SoundSet::Drums,1).data == SNARE_PCM);
  assert(sampleFor(SoundSet::Percussion,0).data == TOM_PCM);
  assert(sampleFor(SoundSet::Wood,1).data == CLAVES_PCM);

  // A single original drum must render every sample exactly, without gain changes.
  for (int row=0; row<2; ++row) {
    SampleMixer mixer;
    Sample sample=sampleFor(SoundSet::Drums,row);
    assert(mixer.trigger(sample));
    std::vector<int16_t> output(sample.length+1);
    mixer.render(output.data(),output.size());
    for (size_t i=0;i<sample.length;++i) assert(output[i] == sample.data[i]);
    assert(output.back()==0);
  }
  // A second hit starts while the preceding sample's tail continues.
  const int16_t a[]={1000,2000,3000,4000}, b[]={100,200};
  SampleMixer overlap;int16_t output[4];
  assert(overlap.trigger({a,4}));overlap.render(output,2);
  assert(output[0]==1000 && output[1]==2000);
  assert(overlap.trigger({b,2}));overlap.render(output,4);
  assert(output[0]==3100 && output[1]==4200 && output[2]==0);
  const int16_t loud[]={30000,-30000};SampleMixer bounded;
  assert(bounded.trigger({loud,2}));assert(bounded.trigger({loud,2}));
  bounded.render(output,2);assert(output[0]==32767 && output[1]==-32768);

  // Both rows on every step at the fastest tempo: no voice exhaustion / dropped triggers.
  SampleMixer dense;std::vector<int16_t> step(44100*30/170);
  for (int i=0;i<80;++i) {
    assert(dense.trigger(sampleFor(SoundSet::Drums,0)));
    assert(dense.trigger(sampleFor(SoundSet::Drums,1)));
    dense.render(step.data(),step.size());
  }
  std::puts("PASS: kit commands, loop boundary, exact drums, overlapping tails, clipping, dense 170 BPM.");
}
