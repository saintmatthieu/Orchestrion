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
#include <async/asyncable.h>
#include <async/channel.h>
#include <audio/audiotypes.h>
#include <audio/internal/worker/track.h>
#include <modularity/ioc.h>
#include <vst/internal/vstaudioclient.h>

namespace muse::vst
{
class VstAudioClient;
}

namespace dgk
{
class IOrchestrionSequencer;

namespace orchestrion
{
class VstTrackAudioInput : public muse::audio::ITrackAudioInput,
                           public muse::Injectable,
                           public muse::async::Asyncable
{
  muse::Inject<IOrchestrion> orchestrion;

public:
  VstTrackAudioInput(muse::vst::VstPluginPtr loadedVstPlugin);

  // ITrackAudioInput
private:
  void seek(const muse::audio::msecs_t newPositionMsecs) override;
  const muse::audio::AudioInputParams &inputParams() const override;
  void applyInputParams(
      const muse::audio::AudioInputParams &requiredParams) override;
  muse::async::Channel<muse::audio::AudioInputParams>
  inputParamsChanged() const override;

  // IAudioSource
private:
  bool isActive() const override;
  void setIsActive(bool arg) override;
  void setSampleRate(unsigned int sampleRate) override;
  unsigned int audioChannelsCount() const override;
  muse::async::Channel<unsigned int> audioChannelsCountChanged() const override;
  muse::audio::samples_t
  process(float *buffer, muse::audio::samples_t samplesPerChannel) override;

private:
  void setupCallback(const IOrchestrionSequencer &);

  muse::audio::AudioInputParams m_inputParams;
  muse::async::Channel<muse::audio::AudioInputParams> m_inputParamsChanged;
  std::unique_ptr<muse::vst::VstAudioClient> m_vstAudioClient;
};
} // namespace orchestrion
} // namespace dgk