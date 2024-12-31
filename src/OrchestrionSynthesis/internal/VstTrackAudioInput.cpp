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
#include "VstTrackAudioInput.h"
#include <log.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <vst/internal/vstaudioclient.h>

namespace dgk
{

namespace
{
constexpr auto channelCount = 2;

Steinberg::Vst::Event toSteinbergEvent(const NoteEvent &noteEvent)
{
  Steinberg::Vst::Event event;
  event.busIndex = 0;
  event.sampleOffset = 0;
  event.ppqPosition = 0;
  event.flags = Steinberg::Vst::Event::kIsLive;
  if (noteEvent.type == NoteEventType::noteOn)
  {
    event.type = Steinberg::Vst::Event::kNoteOnEvent;
    event.noteOn.channel = 0; // TODO
    event.noteOn.pitch = noteEvent.pitch;
    event.noteOn.tuning = 0.f;
    event.noteOn.velocity = noteEvent.velocity;
    event.noteOn.length = 0;
    event.noteOn.noteId = 0; // What's that?
  }
  else
  {
    event.type = Steinberg::Vst::Event::kNoteOffEvent;
    event.noteOff.channel = 0; // TODO
    event.noteOff.tuning = 0.f;
    event.noteOff.pitch = noteEvent.pitch;
    event.noteOff.velocity = noteEvent.velocity;
    event.noteOff.noteId = 0;
  }
  return event;
}

Steinberg::Vst::Event toSteinbergEvent(const PedalEvent &pedalEvent)
{
  return {};
}
} // namespace

VstTrackAudioInput::VstTrackAudioInput(muse::vst::VstPluginPtr loadedVstPlugin)
{
  assert(loadedVstPlugin && loadedVstPlugin->isLoaded());

  m_vstAudioClient = std::make_unique<muse::vst::VstAudioClient>();
  m_vstAudioClient->init(muse::audioplugins::AudioPluginType::Instrument,
                         std::move(loadedVstPlugin), channelCount);
}

void VstTrackAudioInput::processEvent(const EventVariant &event)
{
  IF_ASSERT_FAILED(m_vstAudioClient) { return; }
  if (std::holds_alternative<NoteEvents>(event))
  {
    const auto &noteEvents = std::get<NoteEvents>(event);
    std::for_each(noteEvents.begin(), noteEvents.end(),
                  [this](const NoteEvent &noteEvent)
                  {
                    //
                    m_vstAudioClient->handleEvent(toSteinbergEvent(noteEvent));
                  });
  }
  else if (std::holds_alternative<PedalEvent>(event))
  {
    const auto &pedalEvent = std::get<PedalEvent>(event);
    m_vstAudioClient->handleEvent(toSteinbergEvent(pedalEvent));
  }
  else
  {
    assert(false);
  }
}

bool VstTrackAudioInput::_isActive() const
{
  return m_vstAudioClient != nullptr;
}

void VstTrackAudioInput::_setIsActive(bool arg)
{
  assert(m_vstAudioClient || !arg);
}

void VstTrackAudioInput::_setSampleRate(unsigned int sampleRate)
{
  IF_ASSERT_FAILED(m_vstAudioClient) { return; }
  m_vstAudioClient->setSampleRate(sampleRate);
}

muse::audio::samples_t
VstTrackAudioInput::_process(float *buffer,
                             muse::audio::samples_t samplesPerChannel)
{
  IF_ASSERT_FAILED(m_vstAudioClient) { return 0u; }
  return m_vstAudioClient->process(buffer, samplesPerChannel);
}

} // namespace dgk