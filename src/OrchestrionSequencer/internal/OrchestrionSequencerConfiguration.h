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

#include "IOrchestrionSequencerConfiguration.h"

#include "framework/global/async/asyncable.h"

namespace dgk
{
class OrchestrionSequencerConfiguration
    : public IOrchestrionSequencerConfiguration,
      public muse::async::Asyncable
{
public:
  OrchestrionSequencerConfiguration() = default;
  ~OrchestrionSequencerConfiguration() override = default;

  void init();

private:
  bool velocityRecordingEnabled() const override;
  void setVelocityRecordingEnabled(bool) override;
  muse::async::Notification velocityRecordingEnabledChanged() const override;

  bool noteInfoTooltipEnabled() const override;
  void setNoteInfoTooltipEnabled(bool) override;
  muse::async::Notification noteInfoTooltipEnabledChanged() const override;

  bool tempoVisualizationEnabled() const override;
  void setTempoVisualizationEnabled(bool) override;
  muse::async::Notification tempoVisualizationEnabledChanged() const override;

  bool gradingExposed() const override;
  void setGradingExposed(bool) override;
  muse::async::Notification gradingExposedChanged() const override;

  bool gradingEnabled() const override;
  void setGradingEnabled(bool) override;
  muse::async::Notification gradingEnabledChanged() const override;

  bool persistentTimingMarksEnabled() const override;
  void setPersistentTimingMarksEnabled(bool) override;
  muse::async::Notification
  persistentTimingMarksEnabledChanged() const override;

  bool handSyncScoreEnabled() const override;
  void setHandSyncScoreEnabled(bool) override;
  muse::async::Notification handSyncScoreEnabledChanged() const override;

  bool dynamicsScoreEnabled() const override;
  void setDynamicsScoreEnabled(bool) override;
  muse::async::Notification dynamicsScoreEnabledChanged() const override;

  bool autoPlayExposed() const override;
  void setAutoPlayExposed(bool) override;
  muse::async::Notification autoPlayExposedChanged() const override;
  int autoPlayedStaff() const override;
  void setAutoPlayedStaff(int) override;
  muse::async::Notification autoPlayedStaffChanged() const override;
  bool timeProportionalSpacingEnabled() const override;
  void setTimeProportionalSpacingEnabled(bool) override;
  muse::async::Notification
  timeProportionalSpacingEnabledChanged() const override;

  double tempoSmoothingMemory() const override;
  void setTempoSmoothingMemory(double) override;
  muse::async::Notification tempoSmoothingMemoryChanged() const override;

  bool unrollRepeatsEnabled() const override;
  void setUnrollRepeatsEnabled(bool) override;
  muse::async::Notification unrollRepeatsEnabledChanged() const override;

  bool keyboardHelpDismissed() const override;
  void setKeyboardHelpDismissed(bool) override;

  muse::async::Notification m_velocityRecordingEnabledChanged;
  muse::async::Notification m_noteInfoTooltipEnabledChanged;
  muse::async::Notification m_tempoVisualizationEnabledChanged;
  muse::async::Notification m_gradingExposedChanged;
  muse::async::Notification m_gradingEnabledChanged;
  muse::async::Notification m_persistentTimingMarksEnabledChanged;
  muse::async::Notification m_handSyncScoreEnabledChanged;
  muse::async::Notification m_dynamicsScoreEnabledChanged;
  muse::async::Notification m_autoPlayExposedChanged;
  muse::async::Notification m_autoPlayedStaffChanged;
  muse::async::Notification m_timeProportionalSpacingEnabledChanged;
  muse::async::Notification m_tempoSmoothingMemoryChanged;
  muse::async::Notification m_unrollRepeatsEnabledChanged;
};
} // namespace dgk
