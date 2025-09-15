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
#include "LowpassFilterBank.h"
#include "global/log.h"
#include <array>

namespace dgk
{
namespace
{
constexpr auto maxSamples = 4096;

} // namespace

LowpassFilterBank::LowpassFilterBank(const SynthFactory &synthFactory)
    : m_mixBuffer(maxSamples), m_maxSamples{maxSamples}
{
  for (auto i = 0; i < numVelocitySteps; ++i)
  {
    constexpr auto numPoints = 4;
    constexpr std::array<double, numPoints> velocities{0., 0.25, 0.5, 0.75};
    constexpr std::array<double, numPoints> cutoffs{200., 400., 1000., 7000.};
    // Interpolate cutoff frequency linearly.
    const auto velocity = i / (numVelocitySteps - 1.);
    const auto it =
        std::upper_bound(velocities.begin(), velocities.end(), velocity);
    const auto index = it - velocities.begin();
    const auto cutoff =
        index == 0 ? cutoffs[0]
        : index == numPoints
            ? cutoffs[numPoints - 1]
            : cutoffs[index - 1] +
                  (cutoffs[index] - cutoffs[index - 1]) *
                      (velocity - velocities[index - 1]) /
                      (velocities[index] - velocities[index - 1]);

    m_synthesizers[i] =
        std::make_shared<LowpassFilteredSynthesizer>(synthFactory(), cutoff);
  }
}

int LowpassFilterBank::sampleRate() const
{
  return m_synthesizers[0]->sampleRate();
}

size_t LowpassFilterBank::process(float *buffer, size_t samplesPerChannel)
{
  const auto numSamples = samplesPerChannel * numChannels;
  IF_ASSERT_FAILED(numSamples <= m_maxSamples)
  {
    m_mixBuffer.resize(numSamples);
    m_maxSamples = numSamples;
  }
  std::fill(buffer, buffer + numSamples, 0.f);
  std::for_each(m_synthesizers.begin(), m_synthesizers.end(),
                [&](const auto &synth)
                {
                  synth->process(m_mixBuffer.data(), samplesPerChannel);
                  for (auto i = 0u; i < numSamples; ++i)
                    buffer[i] += m_mixBuffer[i];
                });
  return samplesPerChannel;
}

void LowpassFilterBank::onNoteOns(size_t numNoteons, const TrackIndex *channels,
                                  const int *pitches, const float *velocities)
{
  for (auto i = 0u; i < numNoteons; ++i)
  {
    const auto velocity = velocities[i];
    const auto index = std::min<int>(velocity * (numVelocitySteps - 1) + .5,
                                     numVelocitySteps - 1);

    auto &map = m_pitchesToSynthIndex[channels[i]];
    if (map.count(pitches[i]) && map.at(pitches[i]) != index)
    {
      // Another synth is already playing this note. Stop it.
      const auto otherIndex = map.at(pitches[i]);
      m_synthesizers[otherIndex]->onNoteOffs(1, channels + i, pitches + i);
    }

    m_synthesizers[index]->onNoteOns(1, channels + i, pitches + i,
                                     velocities + i);
    map[pitches[i]] = index;
  }
}

void LowpassFilterBank::onNoteOffs(size_t numNoteoffs,
                                   const TrackIndex *channels,
                                   const int *pitches)
{
  for (auto i = 0u; i < numNoteoffs; ++i)
  {
    auto &map = m_pitchesToSynthIndex[channels[i]];
    const auto it = map.find(pitches[i]);
    if (it == map.end())
      continue;
    m_synthesizers[it->second]->onNoteOffs(1, channels + i, pitches + i);
    map.erase(it);
  }
}

void LowpassFilterBank::onPedal(bool on)
{
  for (auto &synth : m_synthesizers)
    synth->onPedal(on);
}

} // namespace dgk