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
#include "GradingModel.h"
#include "MuseScoreShell/OrchestrionActionIds.h"

namespace dgk
{
GradingModel::GradingModel(QObject *parent) : QObject(parent) {}

void GradingModel::load()
{
  sequencerConfiguration()->gradingEnabledChanged().onNotify(
      this, [this] { emit gradingEnabledChanged(); });
  sequencerConfiguration()->persistentTimingMarksEnabledChanged().onNotify(
      this, [this] { emit persistentMarksChanged(); });
  sequencerConfiguration()->handSyncScoreEnabledChanged().onNotify(
      this, [this] { emit handSyncScoreChanged(); });
  sequencerConfiguration()->dynamicsScoreEnabledChanged().onNotify(
      this, [this] { emit dynamicsScoreChanged(); });
  sequencerConfiguration()->timeProportionalSpacingEnabledChanged().onNotify(
      this, [this] { emit proportionalSpacingChanged(); });
  orchestrion()->playModeChanged().onNotify(this,
                                            [this] { emit playModeChanged(); });

  // The Grading menu's "Settings…" item.
  dispatcher()->reg(this, actionIds::gradingSettings,
                    [this] { emit openSettingsRequested(); });
}

bool GradingModel::gradingEnabled() const
{
  return sequencerConfiguration()->gradingEnabled();
}

void GradingModel::setGradingEnabled(bool enabled)
{
  sequencerConfiguration()->setGradingEnabled(enabled);
}

bool GradingModel::persistentMarks() const
{
  return sequencerConfiguration()->persistentTimingMarksEnabled();
}

void GradingModel::setPersistentMarks(bool enabled)
{
  sequencerConfiguration()->setPersistentTimingMarksEnabled(enabled);
}

bool GradingModel::handSyncScore() const
{
  return sequencerConfiguration()->handSyncScoreEnabled();
}

void GradingModel::setHandSyncScore(bool enabled)
{
  sequencerConfiguration()->setHandSyncScoreEnabled(enabled);
}

bool GradingModel::dynamicsScore() const
{
  return sequencerConfiguration()->dynamicsScoreEnabled();
}

void GradingModel::setDynamicsScore(bool enabled)
{
  sequencerConfiguration()->setDynamicsScoreEnabled(enabled);
}

bool GradingModel::proportionalSpacing() const
{
  return sequencerConfiguration()->timeProportionalSpacingEnabled();
}

void GradingModel::setProportionalSpacing(bool enabled)
{
  sequencerConfiguration()->setTimeProportionalSpacingEnabled(enabled);
}

int GradingModel::playMode() const
{
  return static_cast<int>(orchestrion()->playMode());
}

void GradingModel::setPlayMode(int mode)
{
  orchestrion()->setPlayMode(static_cast<PlayMode>(mode));
}
} // namespace dgk
