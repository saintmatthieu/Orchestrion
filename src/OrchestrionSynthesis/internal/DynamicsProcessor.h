/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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

#include "BuiltInEffectTypes.h"
#include "ParameterStore.h"
#include "thirdparty/DynamicRangeProcessor/CompressorProcessor.h"
#include <async/channel.h>
#include <audio/engine/ifxprocessor.h>

#include <memory>
#include <vector>

namespace dgk
{
/**
 * The built-in compressor or limiter in the audio engine's fx chain:
 * Audacity's CompressorProcessor on the engine's interleaved stereo buffer,
 * with the parameters from its store and the meters for the UI.
 */
class DynamicsProcessor : public muse::audio::IFxProcessor
{
public:
  DynamicsProcessor(
      BuiltInEffect effect, muse::audio::AudioFxParams params,
      std::shared_ptr<const ParameterStore<DynamicRangeProcessorSettings>>
          parameters,
      std::shared_ptr<EffectMeter> meter);

  // IFxProcessor
private:
  muse::audio::AudioFxType type() const override;
  std::string name() const override;
  const muse::audio::AudioFxParams &params() const override;
  muse::async::Channel<muse::audio::AudioFxParams>
  paramsChanged() const override;
  void setOutputSpec(const muse::audio::OutputSpec &spec) override;
  bool active() const override;
  void setActive(bool active) override;
  void setMode(muse::audio::ProcessMode mode) override;
  bool shouldProcessDuringSilence() const override;
  void process(float *buffer, muse::audio::samples_t sampleCount,
               muse::audio::samples_t playbackPositionSamples) override;

private:
  void refreshSettings();

  const BuiltInEffect m_effect;
  muse::audio::AudioFxParams m_params;
  const std::shared_ptr<const ParameterStore<DynamicRangeProcessorSettings>>
      m_parameters;
  const std::shared_ptr<EffectMeter> m_meter;
  muse::async::Channel<muse::audio::AudioFxParams> m_paramsChanged;

  CompressorProcessor m_processor;
  DynamicRangeProcessorSettings m_settings{CompressorSettings{}};
  unsigned m_settingsVersion = 0;
  int m_channelCount = 0; // the channels the processor sees (at most two)
  int m_bufferChannelCount = 0;
  std::vector<std::vector<float>> m_input;
  std::vector<std::vector<float>> m_output;
  std::vector<const float *> m_inputPointers;
  std::vector<float *> m_outputPointers;
};
} // namespace dgk
