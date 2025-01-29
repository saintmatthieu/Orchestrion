#pragma once

#include "IOrchestrionSynthesizer.h"
#include "PolyphonicSynthesizerImpl.h"
#include <audio/isoundfontrepository.h>
#include <fluidsynth/types.h>

namespace dgk
{
class FluidSynthesizer : public IOrchestrionSynthesizer,
                         private PolyphonicSynthesizerImpl
{
  muse::Inject<muse::audio::ISoundFontRepository> soundFontRepository;

public:
  FluidSynthesizer(int sampleRate);
  ~FluidSynthesizer();

  int sampleRate() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const TrackIndex *channels,
                 const int *pitches, const float *velocities) override;
  void onNoteOffs(size_t numNoteons, const TrackIndex *channels,
                  const int *pitches) override;
  void onPedal(bool on) override;

private:
  void onVoicesReset() override;
  void allNotesOff() override;

  const int m_sampleRate;
  bool m_allSet = false;
  fluid_synth_t *m_fluidSynth = nullptr;
  fluid_settings_t *m_fluidSettings = nullptr;
};
} // namespace dgk
