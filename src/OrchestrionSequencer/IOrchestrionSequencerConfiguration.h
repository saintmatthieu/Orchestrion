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

#include "framework/global/async/notification.h"
#include "framework/global/modularity/ioc.h"

namespace dgk
{
class IOrchestrionSequencerConfiguration : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrionSequencerConfiguration);

public:
  virtual ~IOrchestrionSequencerConfiguration() = default;

  virtual bool velocityRecordingEnabled() const = 0;
  virtual void setVelocityRecordingEnabled(bool) = 0;
  virtual muse::async::Notification velocityRecordingEnabledChanged() const = 0;

  //! Debug aid: when enabled, hovering a note shows a tooltip with note
  //! information (currently the chord's score-implied dynamic velocity).
  //! Off by default.
  virtual bool noteInfoTooltipEnabled() const = 0;
  virtual void setNoteInfoTooltipEnabled(bool) = 0;
  virtual muse::async::Notification noteInfoTooltipEnabledChanged() const = 0;

  //! Debug aid: when enabled, a strip beneath the score shows the real-time
  //! tempo model. Off by default.
  virtual bool tempoVisualizationEnabled() const = 0;
  virtual void setTempoVisualizationEnabled(bool) = 0;
  virtual muse::async::Notification
  tempoVisualizationEnabledChanged() const = 0;

  //! Whether the beginner "use your number keys" help has been dismissed.
  //! Persisted so the tip auto-shows only until the user closes it.
  virtual bool keyboardHelpDismissed() const = 0;
  virtual void setKeyboardHelpDismissed(bool) = 0;
};
} // namespace dgk