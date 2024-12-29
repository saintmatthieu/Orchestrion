#pragma once

#include "IOrchestrionSynthesizer.h"
#include <audio/isoundfontrepository.h>
#include <fluidsynth/types.h>
#include <modularity/ioc.h>

namespace dgk
{
class FluidSynthesizer : public IOrchestrionSynthesizer, public muse::Injectable
{
  muse::Inject<muse::audio::ISoundFontRepository> soundFontRepository;

public:
  FluidSynthesizer(int sampleRate);
  ~FluidSynthesizer();

  int sampleRate() const override;
  int numChannels() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const int *pitches,
                 const float *velocities) override;
  void onNoteOffs(size_t numNoteons, const int *pitches) override;
  void onPedal(bool on) override;

private:
  int m_sampleRate = 0;
  fluid_synth_t *m_fluidSynth = nullptr;
  fluid_settings_t *m_fluidSettings = nullptr;
};
} // namespace dgk
