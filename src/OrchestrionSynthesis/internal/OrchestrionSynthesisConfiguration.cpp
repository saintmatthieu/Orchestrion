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
#include "OrchestrionSynthesisConfiguration.h"
#include <global/settings.h>

namespace dgk
{
namespace
{
const std::string module_name("OrchestrionSynthesis");
const muse::Settings::Key SYNTH(module_name, "SYNTH");
const muse::Settings::Key REVERB(module_name, "REVERB");
} // namespace

void OrchestrionSynthesisConfiguration::init()
{
  muse::settings()->setDefaultValue(SYNTH, muse::Val{""});

  synthManager()->selectedSynthChanged().onNotify(
      this,
      [this]
      {
        const auto deviceId = synthManager()->selectedSynth();
        muse::settings()->setLocalValue(SYNTH, muse::Val{deviceId});
      });

  muse::settings()->setDefaultValue(
      REVERB, muse::Val{static_cast<int>(ReverbPreset::Hall)});
  m_reverbPreset->store(muse::settings()->value(REVERB).toInt());
  muse::settings()
      ->valueChanged(REVERB)
      .onReceive(this,
                 [this](const muse::Val &val)
                 {
                   m_reverbPreset->store(val.toInt());
                   m_reverbPresetChanged.notify();
                 });
}

ReverbPreset OrchestrionSynthesisConfiguration::reverbPreset() const
{
  return static_cast<ReverbPreset>(muse::settings()->value(REVERB).toInt());
}

void OrchestrionSynthesisConfiguration::setReverbPreset(ReverbPreset preset)
{
  muse::settings()->setSharedValue(REVERB,
                                   muse::Val{static_cast<int>(preset)});
}

muse::async::Notification
OrchestrionSynthesisConfiguration::reverbPresetChanged() const
{
  return m_reverbPresetChanged;
}

std::shared_ptr<const std::atomic<int>>
OrchestrionSynthesisConfiguration::reverbPresetForAudioThread() const
{
  return m_reverbPreset;
}

void OrchestrionSynthesisConfiguration::postInit()
{
  const std::string value = muse::settings()->value(SYNTH).toString();
  if (!value.empty())
    synthManager()->selectSynth(value);
}
} // namespace dgk