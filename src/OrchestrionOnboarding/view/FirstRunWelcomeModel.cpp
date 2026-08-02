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
#include "FirstRunWelcomeModel.h"
#include "MuseScoreShell/OrchestrionActionIds.h"

namespace dgk
{
FirstRunWelcomeModel::FirstRunWelcomeModel(QObject *parent) : QObject(parent) {}

void FirstRunWelcomeModel::init()
{
  setActive(!configuration()->firstRunWelcomeAcknowledged());
}

void FirstRunWelcomeModel::dismiss(bool dontShowAgain)
{
  if (dontShowAgain)
    configuration()->setFirstRunWelcomeAcknowledged(true);
  setActive(false);
}

void FirstRunWelcomeModel::openFileMenu()
{
  dispatcher()->dispatch(actionIds::openFileMenu);
}

void FirstRunWelcomeModel::closeAppMenu()
{
  dispatcher()->dispatch(actionIds::closeAppMenu);
}

bool FirstRunWelcomeModel::active() const { return m_active; }

void FirstRunWelcomeModel::setActive(bool active)
{
  if (m_active == active)
    return;
  m_active = active;
  emit activeChanged();
}
} // namespace dgk
