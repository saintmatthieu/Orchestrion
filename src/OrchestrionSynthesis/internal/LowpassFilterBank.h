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

#include "LowpassFilteredSynthesizer.h"
#include <array>
#include <functional>

namespace dgk
{
using SynthFactory =
    std::function<std::unique_ptr<IOrchestrionSynthesizer>(void)>;

class LowpassFilterBank : public IOrchestrionSynthesizer
{
public:
  LowpassFilterBank(const SynthFactory &);

private:
  int sampleRate() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const TrackIndex *channels,
                 const int *pitches, const float *velocities) override;
  void onNoteOffs(size_t numNoteoffs, const TrackIndex *channels,
                  const int *pitches) override;
  void onPedal(bool on) override;

  static const auto numVelocitySteps = 9;
  std::array<std::shared_ptr<IOrchestrionSynthesizer>, numVelocitySteps>
      m_synthesizers;
  std::vector<float> m_mixBuffer;

  // TrackIndex must be hashable
  struct TrackIndexHash
  {
    size_t operator()(const TrackIndex &track) const
    {
      return std::hash<int>()(track.value);
    }
  };

  std::unordered_map<TrackIndex, std::unordered_map<int, int>, TrackIndexHash>
      m_pitchesToSynthIndex;
  size_t m_maxSamples;
};
} // namespace dgk