/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include "IOrchestrionSynthesisConfiguration.h"
#include "IOrchestrionSynthesizer.h"
#include "PolyphonicSynthesizerImpl.h"
#include <audio/isoundfontrepository.h>
#include <fluidsynth/types.h>

#include <atomic>
#include <memory>

namespace dgk
{
class FluidSynthesizer : public IOrchestrionSynthesizer,
                         private PolyphonicSynthesizerImpl
{
  muse::Inject<muse::audio::ISoundFontRepository> soundFontRepository;
  muse::Inject<IOrchestrionSynthesisConfiguration> synthesisConfiguration;

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
  void doAllNotesOff() override;

  // Applies the given ReverbPreset to the live synth. Must only be called from
  // the audio thread (the constructor counts, as no rendering happens yet).
  void applyReverb(int preset);

  const int m_sampleRate;
  bool m_allSet = false;
  fluid_synth_t *m_fluidSynth = nullptr;
  fluid_settings_t *m_fluidSettings = nullptr;

  // Lock-free reverb preset shared with the configuration. Polled in process()
  // so a menu change takes effect without recreating the synth.
  std::shared_ptr<const std::atomic<int>> m_reverbPreset;
  int m_appliedReverbPreset = -1;
};
} // namespace dgk
