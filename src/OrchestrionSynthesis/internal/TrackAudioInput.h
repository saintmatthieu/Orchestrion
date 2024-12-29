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

#include "Orchestrion/IOrchestrion.h"
#include "Orchestrion/OrchestrionTypes.h"
#include <async/asyncable.h>
#include <async/channel.h>
#include <audio/audiotypes.h>
#include <audio/internal/worker/track.h>
#include <modularity/ioc.h>

namespace muse::vst
{
class VstAudioClient;
}

namespace dgk
{
class IOrchestrionSequencer;
class TrackAudioInput : public muse::audio::ITrackAudioInput,
                        public muse::Injectable,
                        public muse::async::Asyncable
{
  muse::Inject<dgk::IOrchestrion> orchestrion;

public:
  TrackAudioInput();

private:
  virtual void processEvent(const dgk::EventVariant &event) = 0;
  virtual bool _isActive() const = 0;
  virtual void _setIsActive(bool arg) = 0;
  virtual void _setSampleRate(unsigned int sampleRate) = 0;
  virtual muse::audio::samples_t
  _process(float *buffer, muse::audio::samples_t samplesPerChannel) = 0;

  // ITrackAudioInput
private:
  void seek(const muse::audio::msecs_t newPositionMsecs) final override;
  const muse::audio::AudioInputParams &inputParams() const final override;
  void applyInputParams(
      const muse::audio::AudioInputParams &requiredParams) final override;
  muse::async::Channel<muse::audio::AudioInputParams>
  inputParamsChanged() const final override;

  // IAudioSource
private:
  bool isActive() const final override;
  void setIsActive(bool arg) final override;
  void setSampleRate(unsigned int sampleRate) final override;
  unsigned int audioChannelsCount() const final override;
  muse::async::Channel<unsigned int>
  audioChannelsCountChanged() const final override;
  muse::audio::samples_t
  process(float *buffer,
          muse::audio::samples_t samplesPerChannel) final override;

private:
  void setupCallback(const dgk::IOrchestrionSequencer &);

  muse::audio::AudioInputParams m_inputParams;
  muse::async::Channel<muse::audio::AudioInputParams> m_inputParamsChanged;
};
} // namespace dgk