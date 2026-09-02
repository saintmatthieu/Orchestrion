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
#include "OrchestrionSynthesizerWrapper.h"
#include "OrchestrionSequencer/IOrchestrionSequencer.h"

#include <log.h>

#include <algorithm>
#include <alloca.h>

namespace dgk
{
namespace
{
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
} // namespace

OrchestrionSynthesizerWrapper::OrchestrionSynthesizerWrapper(
    SynthFactory factory, muse::audio::AudioInputParams params)
    : m_params{std::move(params)}, m_synthFactory{std::move(factory)}
{
  orchestrion()->sequencerChanged().onNotify(
      this,
      [this]
      {
        if (const auto sequencer = orchestrion()->sequencer())
          setupCallback(*sequencer);
      });
  if (const auto sequencer = orchestrion()->sequencer())
    setupCallback(*sequencer);
}

void OrchestrionSynthesizerWrapper::setupCallback(
    const IOrchestrionSequencer &sequencer)
{
  sequencer.OutputEvent().onReceive(this, [this](EventVariant event)
                                    { processEvent(event); });
}

void OrchestrionSynthesizerWrapper::processEvent(const EventVariant &event)
{
  if (!m_synthesizer)
    return;
  if (std::holds_alternative<NoteEvents>(event))
  {
    static bool firstNoteLogged = false;
    if (!firstNoteLogged)
    {
      firstNoteLogged = true;
      LOGI() << "First note event received by the Orchestrion synthesizer";
    }
    auto notes = std::get<NoteEvents>(event);
    IF_ASSERT_FAILED(noteoffsAreBeforeNoteons(notes)) { return; }
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
    m_synthesizer->onPedal(pedalEvent.on);
  }
}

void OrchestrionSynthesizerWrapper::sendNoteoffs(const NoteEvent *noteoffs,
                                                 size_t numNoteoffs)
{
  if (numNoteoffs == 0)
    return;
  static_assert(sizeof(TrackIndex) == sizeof(int)); // FYI
  TrackIndex *const channels =
      (TrackIndex *)alloca(numNoteoffs * sizeof(TrackIndex));
  int *const pitches = (int *)alloca(numNoteoffs * sizeof(int));
  for (auto i = 0u; i < numNoteoffs; ++i)
  {
    channels[i].value = noteoffs[i].track.value;
    pitches[i] = noteoffs[i].pitch;
  }
  m_synthesizer->onNoteOffs(numNoteoffs, channels, pitches);
}

void OrchestrionSynthesizerWrapper::sendNoteons(const NoteEvent *noteons,
                                                size_t numNoteons)
{
  if (numNoteons == 0)
    return;
  TrackIndex *const channels =
      (TrackIndex *)alloca(numNoteons * sizeof(TrackIndex));
  int *const pitches = (int *)alloca(numNoteons * sizeof(int));
  float *const velocities = (float *)alloca(numNoteons * sizeof(float));
  for (auto i = 0u; i < numNoteons; ++i)
  {
    channels[i].value = noteons[i].track.value;
    pitches[i] = noteons[i].pitch;
    velocities[i] = noteons[i].velocity;
  }
  m_synthesizer->onNoteOns(numNoteons, channels, pitches, velocities);
}

muse::audio::samples_t
OrchestrionSynthesizerWrapper::process(float *buffer,
                                       muse::audio::samples_t samplesPerChannel)
{
  if (!m_synthesizer)
    return 0;
  static bool renderingLogged = false;
  if (!renderingLogged)
  {
    renderingLogged = true;
    LOGI() << "The audio engine is rendering the Orchestrion synthesizer";
  }
  return m_synthesizer->process(buffer, samplesPerChannel);
}

std::string OrchestrionSynthesizerWrapper::name() const
{
  return "Orchestrion";
}

muse::audio::AudioSourceType OrchestrionSynthesizerWrapper::type() const
{
  return m_params.type();
}

bool OrchestrionSynthesizerWrapper::isValid() const { return true; }

void OrchestrionSynthesizerWrapper::setMode(const muse::audio::ProcessMode mode)
{
  // Orchestrion's synthesizers sound on gestures, whatever the engine's
  // playback state; the mode is only remembered.
  m_mode = mode;
}

muse::audio::ProcessMode OrchestrionSynthesizerWrapper::mode() const
{
  return m_mode;
}

void OrchestrionSynthesizerWrapper::setOutputSpec(
    const muse::audio::OutputSpec &spec)
{
  if (!spec.isValid() || spec == m_outputSpec)
    return;
  const bool sampleRateChanged = spec.sampleRate != m_outputSpec.sampleRate;
  m_outputSpec = spec;
  if (!m_synthesizer || sampleRateChanged)
  {
    m_synthesizer = m_synthFactory(spec);
    LOGI() << "Orchestrion synthesizer " << (m_synthesizer ? "created" : "not available")
           << " (sample rate " << spec.sampleRate << ")";
  }
}

void OrchestrionSynthesizerWrapper::setup(const muse::mpe::PlaybackData &)
{
  // The events come from Orchestrion's sequencer, not from the score's
  // playback data.
}

const muse::mpe::PlaybackData &
OrchestrionSynthesizerWrapper::playbackData() const
{
  static muse::mpe::PlaybackData data;
  return data;
}

const muse::audio::AudioInputParams &
OrchestrionSynthesizerWrapper::params() const
{
  return m_params;
}

muse::async::Channel<muse::audio::AudioInputParams>
OrchestrionSynthesizerWrapper::paramsChanged() const
{
  return m_paramsChanged;
}

muse::audio::TimePosition OrchestrionSynthesizerWrapper::playbackPosition() const
{
  return m_playbackPosition;
}

void OrchestrionSynthesizerWrapper::setPlaybackPosition(
    const muse::audio::TimePosition &position)
{
  m_playbackPosition = position;
}

void OrchestrionSynthesizerWrapper::prepareToPlay() {}

bool OrchestrionSynthesizerWrapper::readyToPlay() const { return true; }

muse::async::Notification
OrchestrionSynthesizerWrapper::readyToPlayChanged() const
{
  return m_readyToPlayChanged;
}

void OrchestrionSynthesizerWrapper::flushSound()
{
  if (m_synthesizer)
    m_synthesizer->allNotesOff();
}

bool OrchestrionSynthesizerWrapper::hasPendingChunks() const { return false; }

void OrchestrionSynthesizerWrapper::processInput() {}

muse::audio::InputProcessingProgress
OrchestrionSynthesizerWrapper::inputProcessingProgress() const
{
  return {};
}

void OrchestrionSynthesizerWrapper::clearCache() {}
} // namespace dgk
