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

#include "TrackAudioInput.h"
#include <DSPFilters/Bessel.h>
#include <audio/isoundfontrepository.h>
#include <fluidsynth/types.h>

namespace dgk
{
class FluidTrackAudioInput : public TrackAudioInput
{
  muse::Inject<muse::audio::ISoundFontRepository> soundFontRepository;

public:
  FluidTrackAudioInput();
  ~FluidTrackAudioInput() override;

private:
  void processEvent(const EventVariant &event) override;
  bool _isActive() const override;
  void _setIsActive(bool arg) override;
  void _setSampleRate(unsigned int sampleRate) override;
  muse::audio::samples_t
  _process(float *buffer, muse::audio::samples_t samplesPerChannel) override;

  void destroySynth();

  fluid_synth_t *m_fluidSynth = nullptr;
  fluid_settings_t *m_fluidSettings = nullptr;
  uint64_t m_sampleRate = 0;
  static const auto lowpassOrder = 2;
  Dsp::SimpleFilter<Dsp::Bessel::LowPass<lowpassOrder>, 2> m_lowPassFilter;
  float** m_audioBuffer = nullptr;
};
} // namespace dgk