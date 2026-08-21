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
#include "AutoPlayModel.h"
#include "MuseScoreShell/OrchestrionActionIds.h"

namespace dgk
{
AutoPlayModel::AutoPlayModel(QObject *parent) : QObject(parent) {}

void AutoPlayModel::load()
{
  sequencerConfiguration()->autoPlayedStaffChanged().onNotify(
      this, [this] { emit autoPlayedStaffChanged(); });

  sequencerConfiguration()->autoPlayExposedChanged().onNotify(
      this, [this] { emit exposedChanged(); });

  dispatcher()->reg(this, actionIds::autoPlaySettings,
                    [this] { emit openSettingsRequested(); });
}

bool AutoPlayModel::exposed() const
{
  return sequencerConfiguration()->autoPlayExposed();
}

int AutoPlayModel::autoPlayedStaff() const
{
  return sequencerConfiguration()->autoPlayedStaff();
}

void AutoPlayModel::setAutoPlayedStaff(int staff)
{
  sequencerConfiguration()->setAutoPlayedStaff(staff);
}
} // namespace dgk
