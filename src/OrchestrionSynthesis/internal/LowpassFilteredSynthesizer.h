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

#include "IOrchestrionSynthesizer.h"
#include <DSPFilters/Bessel.h>
#include <DSPFilters/SmoothedFilter.h>

namespace dgk
{
class LowpassFilteredSynthesizer : public IOrchestrionSynthesizer
{
public:
  LowpassFilteredSynthesizer(std::unique_ptr<IOrchestrionSynthesizer>,
                             double cutoff);
  ~LowpassFilteredSynthesizer();

private:
  int sampleRate() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const TrackIndex* channels, const int *pitches,
                 const float *velocities) override;
  void onNoteOffs(size_t numNoteoffs, const TrackIndex* channels,
                  const int *pitches) override;
  void onPedal(bool on) override;

  void initBuffers(size_t samplesPerChannel);
  void deleteBuffers();

  const std::unique_ptr<IOrchestrionSynthesizer> m_synthesizer;

  static const auto order = 2;
  static const auto audioChannelCount = 2;
  Dsp::SimpleFilter<Dsp::Bessel::LowPass<order>, audioChannelCount>
      m_lowPassFilter;
  float **m_audioBuffer = nullptr;
  size_t m_maxSamplesPerChannel;
};
} // namespace dgk