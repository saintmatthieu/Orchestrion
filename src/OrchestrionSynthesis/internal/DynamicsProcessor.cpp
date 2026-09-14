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
#include "DynamicsProcessor.h"
#include "BuiltInEffectResources.h"

#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
// Audacity's processor tracks the peak of two channels at most.
constexpr int maxProcessedChannels = 2;
} // namespace

DynamicsProcessor::DynamicsProcessor(
    BuiltInEffect effect, muse::audio::AudioFxParams params,
    std::shared_ptr<const ParameterStore<DynamicRangeProcessorSettings>>
        parameters,
    std::shared_ptr<EffectMeter> meter)
    : m_effect{effect}, m_params{std::move(params)},
      m_parameters{std::move(parameters)}, m_meter{std::move(meter)}
{
  refreshSettings();
}

muse::audio::AudioFxType DynamicsProcessor::type() const
{
  return muse::audio::AudioFxType::MuseFx;
}

std::string DynamicsProcessor::name() const { return builtInEffectId(m_effect); }

const muse::audio::AudioFxParams &DynamicsProcessor::params() const
{
  return m_params;
}

muse::async::Channel<muse::audio::AudioFxParams>
DynamicsProcessor::paramsChanged() const
{
  return m_paramsChanged;
}

void DynamicsProcessor::setOutputSpec(const muse::audio::OutputSpec &spec)
{
  if (!spec.isValid())
    return;
  m_bufferChannelCount = static_cast<int>(spec.audioChannelCount);
  m_channelCount = std::min(m_bufferChannelCount, maxProcessedChannels);
  m_input.assign(m_channelCount, std::vector<float>(spec.samplesPerChannel));
  m_output.assign(m_channelCount, std::vector<float>(spec.samplesPerChannel));
  m_inputPointers.resize(m_channelCount);
  m_outputPointers.resize(m_channelCount);
  // The processor caps its own block size and loops over longer blocks.
  m_processor.Init(static_cast<int>(spec.sampleRate), m_channelCount,
                   static_cast<int>(spec.samplesPerChannel));
}

bool DynamicsProcessor::active() const { return m_params.active; }

void DynamicsProcessor::setActive(bool active) { m_params.active = active; }

void DynamicsProcessor::setMode(muse::audio::ProcessMode) {}

bool DynamicsProcessor::shouldProcessDuringSilence() const { return false; }

void DynamicsProcessor::refreshSettings()
{
  if (m_parameters->readIfChanged(m_settings, m_settingsVersion))
    m_processor.ApplySettingsIfNeeded(m_settings);
}

void DynamicsProcessor::process(float *buffer,
                                muse::audio::samples_t sampleCount,
                                muse::audio::samples_t)
{
  if (!buffer || m_channelCount == 0 || sampleCount == 0)
    return;
  refreshSettings();

  // Room for a block longer than announced; not expected, but cheap to allow.
  for (int c = 0; c < m_channelCount; ++c)
  {
    if (m_input[c].size() < sampleCount)
    {
      m_input[c].resize(sampleCount);
      m_output[c].resize(sampleCount);
    }
    m_inputPointers[c] = m_input[c].data();
    m_outputPointers[c] = m_output[c].data();
  }

  float inputPeak = 0.f;
  for (muse::audio::samples_t i = 0; i < sampleCount; ++i)
    for (int c = 0; c < m_channelCount; ++c)
    {
      const float sample = buffer[i * m_bufferChannelCount + c];
      m_input[c][i] = sample;
      inputPeak = std::max(inputPeak, std::abs(sample));
    }

  m_processor.Process(m_inputPointers.data(), m_outputPointers.data(),
                      static_cast<int>(sampleCount));

  float outputPeak = 0.f;
  for (muse::audio::samples_t i = 0; i < sampleCount; ++i)
    for (int c = 0; c < m_channelCount; ++c)
    {
      const float sample = m_output[c][i];
      buffer[i * m_bufferChannelCount + c] = sample;
      outputPeak = std::max(outputPeak, std::abs(sample));
    }

  // The envelope value at the loudest input sample of the block is the gain
  // applied there, before make-up: minus the gain reduction.
  const float gainReductionDb =
      std::max(0.f, -m_processor.GetLastFrameStats().dbGainOfMaxInputSample);
  m_meter->inputPeak.store(inputPeak, std::memory_order_relaxed);
  m_meter->outputPeak.store(outputPeak, std::memory_order_relaxed);
  m_meter->gainReductionDb.store(gainReductionDb, std::memory_order_relaxed);
  if (outputPeak > 1.f)
    m_meter->clipped.store(true, std::memory_order_relaxed);
}
} // namespace dgk
