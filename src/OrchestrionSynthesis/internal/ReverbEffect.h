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

#include "BuiltInEffectRuntime.h"
#include <audio/engine/ifxprocessor.h>
#include <engine/internal/fx/reverb/reverbprocessor.h>

#include <map>
#include <memory>
#include <string>

namespace dgk
{
/**
 * MuseScore's reverb in Orchestrion's chain: the framework's processor, with
 * its parameters driven from the store (MuseScore runs it on fixed defaults)
 * and the meters for the UI.
 */
class ReverbEffect : public muse::audio::IFxProcessor
{
public:
  ReverbEffect(muse::audio::AudioFxParams params,
               std::shared_ptr<const ParameterStore<ReverbParameters>> parameters,
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
  void refreshParameters();

  muse::audio::fx::ReverbProcessor m_reverb;
  const std::shared_ptr<const ParameterStore<ReverbParameters>> m_parameters;
  const std::shared_ptr<EffectMeter> m_meter;
  std::map<std::string, int32_t> m_indexByName;
  ReverbParameters m_values;
  unsigned m_version = 0;
  int m_channelCount = 0;
};
} // namespace dgk
