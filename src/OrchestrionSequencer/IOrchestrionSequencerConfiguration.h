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

  //! When enabled, each played note gets a colored flash judging its timing
  //! against the performer's own tempo curve (gold = perfect, green = good,
  //! blue = early, red = late). On by default.
  virtual bool timingFeedbackEnabled() const = 0;
  virtual void setTimingFeedbackEnabled(bool) = 0;
  virtual muse::async::Notification timingFeedbackEnabledChanged() const = 0;

  //! When enabled, the timing marks don't fade out: they stay on the page
  //! until the stats reset, so a whole take can be reviewed after playing it.
  //! Off by default.
  virtual bool persistentTimingMarksEnabled() const = 0;
  virtual void setPersistentTimingMarksEnabled(bool) = 0;
  virtual muse::async::Notification
  persistentTimingMarksEnabledChanged() const = 0;

  //! Whether the performance score includes the hand-synchronization
  //! component. Off by default: hand asynchrony correlates strongly with the
  //! tempo-smoothness errors anyway, so one score usually suffices.
  virtual bool handSyncScoreEnabled() const = 0;
  virtual void setHandSyncScoreEnabled(bool) = 0;
  virtual muse::async::Notification handSyncScoreEnabledChanged() const = 0;

  //! Whether the performance score includes the dynamics-smoothness
  //! component (played velocities judged against the performer's own
  //! smoothed loudness curve). On by default; it only contributes when the
  //! input device measures velocity.
  virtual bool dynamicsScoreEnabled() const = 0;
  virtual void setDynamicsScoreEnabled(bool) = 0;
  virtual muse::async::Notification dynamicsScoreEnabledChanged() const = 0;

  //! Which hand the machine plays, following the performer's tempo: −1 =
  //! none, 0 = the right hand (staff 0), 1 = the left. The auto-played hand
  //! is exempt from timing judgments and scoring.
  virtual int autoPlayedStaff() const = 0;
  virtual void setAutoPlayedStaff(int) = 0;
  virtual muse::async::Notification autoPlayedStaffChanged() const = 0;

  //! When enabled, the score is laid out time-proportionally: horizontal
  //! note spacing is determined by durations alone, so equal distance =
  //! equal musical time (the canvas for the tempo-warped note overlays).
  //! Off by default.
  virtual bool timeProportionalSpacingEnabled() const = 0;
  virtual void setTimeProportionalSpacingEnabled(bool) = 0;
  virtual muse::async::Notification
  timeProportionalSpacingEnabledChanged() const = 0;

  //! The tempo model's smoothing memory γ ∈ (0,1): how stiff the fitted
  //! tempo spline is (higher = smoother, less tolerant of short-term
  //! deviations). Tunable from the post-take slider; default 0.6.
  virtual double tempoSmoothingMemory() const = 0;
  virtual void setTempoSmoothingMemory(double) = 0;
  virtual muse::async::Notification tempoSmoothingMemoryChanged() const = 0;

  //! Whether the beginner "use your number keys" help has been dismissed.
  //! Persisted so the tip auto-shows only until the user closes it.
  virtual bool keyboardHelpDismissed() const = 0;
  virtual void setKeyboardHelpDismissed(bool) = 0;
};
} // namespace dgk