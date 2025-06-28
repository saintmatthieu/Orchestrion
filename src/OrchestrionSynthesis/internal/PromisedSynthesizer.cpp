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
#include "PromisedSynthesizer.h"

namespace dgk
{
PromisedSynthesizer::PromisedSynthesizer(SynthPromise promise)
    : m_promise{std::move(promise)}
{
  m_promise.onReceive(
      this, [this](std::shared_ptr<IOrchestrionSynthesizer> synthesizer)
      { m_synthesizer = std::move(synthesizer); });
}

int PromisedSynthesizer::sampleRate() const
{
  return m_synthesizer ? m_synthesizer->sampleRate() : 0;
}

size_t PromisedSynthesizer::process(float *buffer, size_t samplesPerChannel)
{
  return m_synthesizer ? m_synthesizer->process(buffer, samplesPerChannel) : 0;
}

void PromisedSynthesizer::onNoteOns(size_t numNoteons, const TrackIndex* channels,
                                    const int *pitches, const float *velocities)
{
  if (m_synthesizer)
    m_synthesizer->onNoteOns(numNoteons, channels, pitches, velocities);
}

void PromisedSynthesizer::onNoteOffs(size_t numNoteoffs, const TrackIndex* channels,
                                     const int *pitches)
{
  if (m_synthesizer)
    m_synthesizer->onNoteOffs(numNoteoffs, channels, pitches);
}

void PromisedSynthesizer::onPedal(bool on)
{
  if (m_synthesizer)
    m_synthesizer->onPedal(on);
}

} // namespace dgk