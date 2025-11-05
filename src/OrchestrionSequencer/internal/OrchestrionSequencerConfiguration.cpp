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
#include "OrchestrionSequencerConfiguration.h"

#include "framework/global/settings.h"

namespace dgk
{
namespace
{
const std::string module_name("OrchestrionSequencer");
const muse::Settings::Key
    VELOCITY_RECORDING_ENABLED(module_name, "VELOCITY_RECORDING_ENABLED");
} // namespace

void OrchestrionSequencerConfiguration::init()
{
  muse::settings()->setDefaultValue(VELOCITY_RECORDING_ENABLED,
                                    muse::Val{false});
  muse::settings()
      ->valueChanged(VELOCITY_RECORDING_ENABLED)
      .onReceive(this, [this](const muse::Val &)
                 { m_velocityRecordingEnabledChanged.notify(); });
}

bool OrchestrionSequencerConfiguration::velocityRecordingEnabled() const
{
  return muse::settings()->value(VELOCITY_RECORDING_ENABLED).toBool();
}

void OrchestrionSequencerConfiguration::setVelocityRecordingEnabled(
    bool enabled)
{
  muse::settings()->setSharedValue(VELOCITY_RECORDING_ENABLED,
                                   muse::Val{enabled});
}

muse::async::Notification
OrchestrionSequencerConfiguration::velocityRecordingEnabledChanged() const
{
  return m_velocityRecordingEnabledChanged;
}

} // namespace dgk
