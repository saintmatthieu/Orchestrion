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
#include "FluidTrackAudioInput.h"
#include "FluidSynthesizer.h"
#include "LowpassFilteredSynthesizer.h"
#include <log.h>

namespace dgk
{
namespace
{
constexpr auto fluidPolicy = ITrackChannelMapper::Policy::oneChannelPerStaff;
constexpr auto maxSamples = 4096;

bool noteoffsAreBeforeNoteons(const NoteEvents &notes)
{
  const auto firstNoteOff =
      std::find_if(notes.begin(), notes.end(), [](const auto &evt)
                   { return evt.type == NoteEventType::noteOff; });
  const auto firstNoteOn =
      std::find_if(notes.begin(), notes.end(), [](const auto &evt)
                   { return evt.type == NoteEventType::noteOn; });
  return firstNoteOff == notes.end() || firstNoteOff < firstNoteOn;
}

bool allFromSameStaff(const NoteEvents &notes)
{
  const auto ref = notes[0].track.staffIndex();
  return std::all_of(notes.begin(), notes.end(), [&](const NoteEvent &note)
                     { return note.track.staffIndex() == ref; });
}
} // namespace

FluidTrackAudioInput::FluidTrackAudioInput()
    : m_mixBuffer(maxSamples), m_maxSamples{maxSamples}
{
}

void FluidTrackAudioInput::processEvent(const EventVariant &event)
{
  if (std::holds_alternative<NoteEvents>(event))
  {
    auto notes = std::get<NoteEvents>(event);
    IF_ASSERT_FAILED(noteoffsAreBeforeNoteons(notes)) { return; }
    IF_ASSERT_FAILED(allFromSameStaff(notes)) { return; }
    const int noteonOffset =
        std::find_if(notes.begin(), notes.end(), [](const auto &evt)
                     { return evt.type == NoteEventType::noteOn; }) -
        notes.begin();

    sendNoteoffs(notes.data(), noteonOffset);
    sendNoteons(notes.data() + noteonOffset, notes.size() - noteonOffset);
  }
  else if (std::holds_alternative<PedalEvent>(event))
  {
    const auto &pedalEvent = std::get<PedalEvent>(event);
    const auto channels =
        mapper()->channelsForInstrument(pedalEvent.instrument, fluidPolicy);
    for (const auto channel : channels)
      m_synthesizers[channel]->onPedal(pedalEvent.on);
  }
}

void FluidTrackAudioInput::sendNoteoffs(const NoteEvent *noteoffs,
                                        size_t numNoteoffs)
{
  if (numNoteoffs == 0)
    return;
  const auto channel =
      mapper()->channelForTrack(noteoffs[0].track, fluidPolicy);
  int *const pitches = (int *)alloca(numNoteoffs * sizeof(int));
  for (auto i = 0; i < numNoteoffs; ++i)
    pitches[i] = noteoffs[i].pitch;
  m_synthesizers[channel]->onNoteOffs(numNoteoffs, pitches);
}

void FluidTrackAudioInput::sendNoteons(const NoteEvent *noteons,
                                       size_t numNoteons)
{
  if (numNoteons == 0)
    return;
  const auto channel = mapper()->channelForTrack(noteons[0].track, fluidPolicy);
  int *const pitches = (int *)alloca(numNoteons * sizeof(int));
  float *const velocities = (float *)alloca(numNoteons * sizeof(float));
  for (auto i = 0; i < numNoteons; ++i)
  {
    pitches[i] = noteons[i].pitch;
    velocities[i] = noteons[i].velocity;
  }
  m_synthesizers[channel]->onNoteOns(numNoteons, pitches, velocities);
}

bool FluidTrackAudioInput::_isActive() const { return !m_synthesizers.empty(); }

void FluidTrackAudioInput::_setIsActive(bool) { assert(false); }

void FluidTrackAudioInput::_setSampleRate(unsigned int sampleRate)
{
  if (m_sampleRate == sampleRate || sampleRate == 0)
    return;

  m_synthesizers.clear();
  for (auto i = 0; i < mapper()->numChannels(fluidPolicy); ++i)
    m_synthesizers.emplace_back(std::make_unique<LowpassFilteredSynthesizer>(
        std::make_unique<FluidSynthesizer>(sampleRate)));
}

muse::audio::samples_t
FluidTrackAudioInput::_process(float *buffer,
                               muse::audio::samples_t samplesPerChannel)
{
  const auto numSamples = samplesPerChannel * m_synthesizers[0]->numChannels();
  IF_ASSERT_FAILED(numSamples <= m_maxSamples)
  {
    m_mixBuffer.resize(numSamples);
    m_maxSamples = numSamples;
  }
  std::fill(buffer, buffer + numSamples, 0.f);
  for (auto i = 0; i < m_synthesizers.size(); ++i)
  {
    m_synthesizers[i]->process(m_mixBuffer.data(), samplesPerChannel);
    for (auto j = 0; j < numSamples; ++j)
      buffer[j] += m_mixBuffer[j];
  }
  return samplesPerChannel;
}
} // namespace dgk