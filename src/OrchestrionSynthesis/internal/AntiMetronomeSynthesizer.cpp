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
#include "AntiMetronomeSynthesizer.h"

namespace dgk
{
AntiMetronomeSynthesizer::AntiMetronomeSynthesizer(int sampleRate,
                                                   muse::audio::TrackId trackId,
                                                   SynthesizerFactory factory)
    : m_sampleRate{sampleRate}, m_trackId{std::move(trackId)},
      m_factory{std::move(factory)}
{
  playbackController()->trackAdded().onReceive(
      this,
      [this, factory = std::move(factory)](muse::audio::TrackId trackId)
      {
        if (trackId != m_trackId)
          return;
        SetOrResetSynth();
      });
  SetOrResetSynth();
}

void AntiMetronomeSynthesizer::SetOrResetSynth()
{
  const std::unordered_map<mu::engraving::InstrumentTrackId,
                           muse::audio::TrackId> &instrumentTrackIdMap =
      playbackController()->instrumentTrackIdMap();
  const auto it = std::find_if(
      instrumentTrackIdMap.begin(), instrumentTrackIdMap.end(),
      [this](const auto &pair) { return pair.second == m_trackId; });
  if (it == instrumentTrackIdMap.end())
    return;
  const mu::engraving::InstrumentTrackId &track = it->first;
  if (track.instrumentId.toLower() == "metronome")
    m_synthesizer.reset();
  else
    m_synthesizer = m_factory(m_sampleRate);
}

int AntiMetronomeSynthesizer::sampleRate() const { return m_sampleRate; }

size_t AntiMetronomeSynthesizer::process(float *buffer,
                                         size_t samplesPerChannel)
{
  if (m_synthesizer)
    return m_synthesizer->process(buffer, samplesPerChannel);
  std::fill(buffer, buffer + samplesPerChannel, 0.f);
  return samplesPerChannel;
}

void AntiMetronomeSynthesizer::onNoteOns(size_t numNoteons, const int *pitches,
                                         const float *velocities)
{
  if (m_synthesizer)
    m_synthesizer->onNoteOns(numNoteons, pitches, velocities);
}

void AntiMetronomeSynthesizer::onNoteOffs(size_t numNoteoffs,
                                          const int *pitches)
{
  if (m_synthesizer)
    m_synthesizer->onNoteOffs(numNoteoffs, pitches);
}

void AntiMetronomeSynthesizer::onPedal(bool on)
{
  if (m_synthesizer)
    m_synthesizer->onPedal(on);
}

} // namespace dgk