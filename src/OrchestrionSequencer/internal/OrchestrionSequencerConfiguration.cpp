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
const muse::Settings::Key
    NOTE_INFO_TOOLTIP_ENABLED(module_name, "NOTE_INFO_TOOLTIP_ENABLED");
const muse::Settings::Key
    TEMPO_VISUALIZATION_ENABLED(module_name, "TEMPO_VISUALIZATION_ENABLED");
const muse::Settings::Key TIMING_FEEDBACK_ENABLED(module_name,
                                                  "TIMING_FEEDBACK_ENABLED");
const muse::Settings::Key KEYBOARD_HELP_DISMISSED(module_name,
                                                  "KEYBOARD_HELP_DISMISSED");
} // namespace

void OrchestrionSequencerConfiguration::init()
{
  muse::settings()->setDefaultValue(VELOCITY_RECORDING_ENABLED,
                                    muse::Val{false});
  muse::settings()
      ->valueChanged(VELOCITY_RECORDING_ENABLED)
      .onReceive(this, [this](const muse::Val &)
                 { m_velocityRecordingEnabledChanged.notify(); });

  muse::settings()->setDefaultValue(NOTE_INFO_TOOLTIP_ENABLED,
                                    muse::Val{false});
  muse::settings()
      ->valueChanged(NOTE_INFO_TOOLTIP_ENABLED)
      .onReceive(this, [this](const muse::Val &)
                 { m_noteInfoTooltipEnabledChanged.notify(); });

  muse::settings()->setDefaultValue(TEMPO_VISUALIZATION_ENABLED,
                                    muse::Val{false});
  muse::settings()
      ->valueChanged(TEMPO_VISUALIZATION_ENABLED)
      .onReceive(this, [this](const muse::Val &)
                 { m_tempoVisualizationEnabledChanged.notify(); });

  muse::settings()->setDefaultValue(TIMING_FEEDBACK_ENABLED, muse::Val{true});
  muse::settings()
      ->valueChanged(TIMING_FEEDBACK_ENABLED)
      .onReceive(this, [this](const muse::Val &)
                 { m_timingFeedbackEnabledChanged.notify(); });

  muse::settings()->setDefaultValue(KEYBOARD_HELP_DISMISSED, muse::Val{false});
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

bool OrchestrionSequencerConfiguration::noteInfoTooltipEnabled() const
{
  return muse::settings()->value(NOTE_INFO_TOOLTIP_ENABLED).toBool();
}

void OrchestrionSequencerConfiguration::setNoteInfoTooltipEnabled(bool enabled)
{
  muse::settings()->setSharedValue(NOTE_INFO_TOOLTIP_ENABLED,
                                   muse::Val{enabled});
}

muse::async::Notification
OrchestrionSequencerConfiguration::noteInfoTooltipEnabledChanged() const
{
  return m_noteInfoTooltipEnabledChanged;
}

bool OrchestrionSequencerConfiguration::tempoVisualizationEnabled() const
{
  return muse::settings()->value(TEMPO_VISUALIZATION_ENABLED).toBool();
}

void OrchestrionSequencerConfiguration::setTempoVisualizationEnabled(
    bool enabled)
{
  muse::settings()->setSharedValue(TEMPO_VISUALIZATION_ENABLED,
                                   muse::Val{enabled});
}

muse::async::Notification
OrchestrionSequencerConfiguration::tempoVisualizationEnabledChanged() const
{
  return m_tempoVisualizationEnabledChanged;
}

bool OrchestrionSequencerConfiguration::timingFeedbackEnabled() const
{
  return muse::settings()->value(TIMING_FEEDBACK_ENABLED).toBool();
}

void OrchestrionSequencerConfiguration::setTimingFeedbackEnabled(bool enabled)
{
  muse::settings()->setSharedValue(TIMING_FEEDBACK_ENABLED, muse::Val{enabled});
}

muse::async::Notification
OrchestrionSequencerConfiguration::timingFeedbackEnabledChanged() const
{
  return m_timingFeedbackEnabledChanged;
}

bool OrchestrionSequencerConfiguration::keyboardHelpDismissed() const
{
  return muse::settings()->value(KEYBOARD_HELP_DISMISSED).toBool();
}

void OrchestrionSequencerConfiguration::setKeyboardHelpDismissed(bool dismissed)
{
  muse::settings()->setSharedValue(KEYBOARD_HELP_DISMISSED,
                                   muse::Val{dismissed});
}

} // namespace dgk
