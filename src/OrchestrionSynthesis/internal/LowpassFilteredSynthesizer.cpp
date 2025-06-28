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
#include "LowpassFilteredSynthesizer.h"

namespace dgk
{
namespace
{
constexpr auto leastTimeBetweenReparametrizations = 0.05;
constexpr auto maxSamplesPerChannel = 2048;
} // namespace

LowpassFilteredSynthesizer::LowpassFilteredSynthesizer(
    std::unique_ptr<IOrchestrionSynthesizer> synthesizer, double cutoff)
    : m_synthesizer{std::move(synthesizer)},
      m_maxSamplesPerChannel{maxSamplesPerChannel}
{
  m_lowPassFilter.setup(order, m_synthesizer->sampleRate(), cutoff);
  initBuffers(maxSamplesPerChannel);
}

LowpassFilteredSynthesizer::~LowpassFilteredSynthesizer() { deleteBuffers(); }

void LowpassFilteredSynthesizer::initBuffers(size_t samplesPerChannel)
{
  m_audioBuffer = new float *[numChannels];
  for (auto i = 0; i < numChannels; ++i)
    m_audioBuffer[i] = new float[maxSamplesPerChannel];
  m_maxSamplesPerChannel = samplesPerChannel;
}

void LowpassFilteredSynthesizer::deleteBuffers()
{
  for (auto i = 0; i < numChannels; ++i)
    delete[] m_audioBuffer[i];
  delete[] m_audioBuffer;
}

int LowpassFilteredSynthesizer::sampleRate() const
{
  return m_synthesizer->sampleRate();
}

size_t LowpassFilteredSynthesizer::process(float *buffer,
                                           size_t samplesPerChannel)
{
  IF_ASSERT_FAILED(samplesPerChannel <= m_maxSamplesPerChannel)
  {
    deleteBuffers();
    initBuffers(samplesPerChannel);
  }

  const auto produced = m_synthesizer->process(buffer, samplesPerChannel);
  const auto C = numChannels;

  // Deinterleave
  for (auto i = 0; i < C; ++i)
    for (muse::audio::samples_t j = 0; j < samplesPerChannel; ++j)
      m_audioBuffer[i][j] = buffer[j * C + i];

  m_lowPassFilter.process((int)samplesPerChannel, m_audioBuffer);

  // Re-Interleave
  for (auto i = 0; i < C; ++i)
    for (muse::audio::samples_t j = 0; j < samplesPerChannel; ++j)
      buffer[j * C + i] = m_audioBuffer[i][j];

  return produced;
}

void LowpassFilteredSynthesizer::onNoteOns(size_t numNoteons,
                                           const TrackIndex* channels,
                                           const int *pitches,
                                           const float *velocities)
{
  m_synthesizer->onNoteOns(numNoteons, channels, pitches, velocities);
}

void LowpassFilteredSynthesizer::onNoteOffs(size_t numNoteoffs,
                                            const TrackIndex* channels,
                                            const int *pitches)
{
  m_synthesizer->onNoteOffs(numNoteoffs, channels, pitches);
}

void LowpassFilteredSynthesizer::onPedal(bool on)
{
  m_synthesizer->onPedal(on);
}

} // namespace dgk