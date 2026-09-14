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
#include "ReverbEffect.h"
#include <log.h>

#include <algorithm>
#include <cmath>

namespace dgk
{
ReverbEffect::ReverbEffect(
    muse::audio::AudioFxParams params,
    std::shared_ptr<const ParameterStore<ReverbParameters>> parameters,
    std::shared_ptr<EffectMeter> meter)
    : m_reverb{params}, m_parameters{std::move(parameters)},
      m_meter{std::move(meter)}
{
  for (int32_t i = 0; i < muse::audio::fx::ReverbProcessor::NumParams; ++i)
  {
    muse::audio::fx::ReverbProcessor::ParameterInfo info;
    m_reverb.getParameterInfo(i, info);
    m_indexByName.emplace(info.name, i);
  }
  refreshParameters();
}

muse::audio::AudioFxType ReverbEffect::type() const { return m_reverb.type(); }

std::string ReverbEffect::name() const { return m_reverb.name(); }

const muse::audio::AudioFxParams &ReverbEffect::params() const
{
  return m_reverb.params();
}

muse::async::Channel<muse::audio::AudioFxParams>
ReverbEffect::paramsChanged() const
{
  return m_reverb.paramsChanged();
}

void ReverbEffect::setOutputSpec(const muse::audio::OutputSpec &spec)
{
  if (!spec.isValid())
    return;
  m_channelCount = static_cast<int>(spec.audioChannelCount);
  m_reverb.setOutputSpec(spec);
}

bool ReverbEffect::active() const { return m_reverb.active(); }

void ReverbEffect::setActive(bool active) { m_reverb.setActive(active); }

void ReverbEffect::setMode(muse::audio::ProcessMode mode)
{
  m_reverb.setMode(mode);
}

bool ReverbEffect::shouldProcessDuringSilence() const
{
  return m_reverb.shouldProcessDuringSilence();
}

void ReverbEffect::refreshParameters()
{
  if (!m_parameters->readIfChanged(m_values, m_version))
    return;
  for (const auto &[name, value] : m_values)
  {
    const auto it = m_indexByName.find(name);
    if (it == m_indexByName.end())
      continue; // reported when the file is read
    m_reverb.setParameter(it->second, value);
  }
}

void ReverbEffect::process(float *buffer, muse::audio::samples_t sampleCount,
                           muse::audio::samples_t playbackPositionSamples)
{
  if (!buffer || m_channelCount == 0 || sampleCount == 0)
    return;
  refreshParameters();

  const size_t count = static_cast<size_t>(sampleCount) * m_channelCount;
  float inputPeak = 0.f;
  for (size_t i = 0; i < count; ++i)
    inputPeak = std::max(inputPeak, std::abs(buffer[i]));

  m_reverb.process(buffer, sampleCount, playbackPositionSamples);

  float outputPeak = 0.f;
  for (size_t i = 0; i < count; ++i)
    outputPeak = std::max(outputPeak, std::abs(buffer[i]));

  m_meter->inputPeak.store(inputPeak, std::memory_order_relaxed);
  m_meter->outputPeak.store(outputPeak, std::memory_order_relaxed);
  if (outputPeak > 1.f)
    m_meter->clipped.store(true, std::memory_order_relaxed);
}
} // namespace dgk
