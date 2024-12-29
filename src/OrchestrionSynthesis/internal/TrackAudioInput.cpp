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
#include "TrackAudioInput.h"
#include "Orchestrion/IOrchestrionSequencer.h"
#include <audioplugins/audiopluginstypes.h>
#include <log.h>

namespace dgk
{
TrackAudioInput::TrackAudioInput()
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

void TrackAudioInput::setupCallback(const IOrchestrionSequencer &sequencer)
{
  sequencer.OutputEvent().onReceive(this, [this](EventVariant event)
                                    { processEvent(event); });
}

void TrackAudioInput::seek(const muse::audio::msecs_t)
{
  // What do I do here?
  assert(false);
}

const muse::audio::AudioInputParams &TrackAudioInput::inputParams() const
{
  return m_inputParams;
}

void TrackAudioInput::applyInputParams(
    const muse::audio::AudioInputParams &params)
{
  assert(false); // What do I do here?
  m_inputParams = params;
}

muse::async::Channel<muse::audio::AudioInputParams>
TrackAudioInput::inputParamsChanged() const
{
  return m_inputParamsChanged;
}

bool TrackAudioInput::isActive() const { return _isActive(); }

void TrackAudioInput::setIsActive(bool arg) { _setIsActive(arg); }

void TrackAudioInput::setSampleRate(unsigned int sampleRate)
{
  _setSampleRate(sampleRate);
}

unsigned int TrackAudioInput::audioChannelsCount() const { return 2; }

muse::async::Channel<unsigned int>
TrackAudioInput::audioChannelsCountChanged() const
{
  static muse::async::Channel<unsigned int> nullChannel;
  return nullChannel;
}

muse::audio::samples_t
TrackAudioInput::process(float *buffer,
                         muse::audio::samples_t samplesPerChannel)
{
  return _process(buffer, samplesPerChannel);
}
} // namespace dgk