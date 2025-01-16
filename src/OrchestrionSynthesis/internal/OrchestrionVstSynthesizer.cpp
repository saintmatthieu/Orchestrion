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
#include "OrchestrionVstSynthesizer.h"
#include <log.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <vst/internal/vstaudioclient.h>

namespace dgk
{

namespace
{
constexpr auto channelCount = 2;

Steinberg::Vst::Event toSteinbergEvent(NoteEventType type, int pitch,
                                       float velocity)
{
  Steinberg::Vst::Event event;
  event.busIndex = 0;
  event.sampleOffset = 0;
  event.ppqPosition = 0;
  event.flags = Steinberg::Vst::Event::kIsLive;
  if (type == NoteEventType::noteOn)
  {
    event.type = Steinberg::Vst::Event::kNoteOnEvent;
    event.noteOn.channel = 0; // TODO
    event.noteOn.pitch = pitch;
    event.noteOn.tuning = 0.f;
    event.noteOn.velocity = velocity;
    event.noteOn.length = 0;
    event.noteOn.noteId = 0; // What's that?
  }
  else
  {
    event.type = Steinberg::Vst::Event::kNoteOffEvent;
    event.noteOff.channel = 0; // TODO
    event.noteOff.tuning = 0.f;
    event.noteOff.pitch = pitch;
    event.noteOff.velocity = velocity;
    event.noteOff.noteId = 0;
  }
  return event;
}

Steinberg::Vst::Event toSteinbergEvent(const PedalEvent &pedalEvent)
{
  return {};
}
} // namespace

OrchestrionVstSynthesizer::OrchestrionVstSynthesizer(
    muse::vst::VstPluginPtr loadedVstPlugin, int sampleRate)
    : m_sampleRate{sampleRate}
{
  assert(loadedVstPlugin && loadedVstPlugin->isLoaded());

  m_vstAudioClient = std::make_unique<muse::vst::VstAudioClient>();
  m_vstAudioClient->init(muse::audioplugins::AudioPluginType::Instrument,
                         std::move(loadedVstPlugin), channelCount);
  m_vstAudioClient->setSampleRate(sampleRate);
}

int OrchestrionVstSynthesizer::sampleRate() const { return m_sampleRate; }

size_t OrchestrionVstSynthesizer::process(float *buffer,
                                          size_t samplesPerChannel)
{
  IF_ASSERT_FAILED(m_vstAudioClient) { return 0u; }
  return m_vstAudioClient->process(buffer, samplesPerChannel);
}

void OrchestrionVstSynthesizer::onNoteOns(size_t numNoteons, const int *pitches,
                                          const float *velocities)
{
  for (size_t i = 0; i < numNoteons; ++i)
    m_vstAudioClient->handleEvent(
        toSteinbergEvent(NoteEventType::noteOn, pitches[i], velocities[i]));
}

void OrchestrionVstSynthesizer::onNoteOffs(size_t numNoteoffs,
                                           const int *pitches)
{
  for (size_t i = 0; i < numNoteoffs; ++i)
    m_vstAudioClient->handleEvent(
        toSteinbergEvent(NoteEventType::noteOff, pitches[i], 0.f));
}

void OrchestrionVstSynthesizer::onPedal(bool on)
{
  // TODO
  assert(false);
}

} // namespace dgk