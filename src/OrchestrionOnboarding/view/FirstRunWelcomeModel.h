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

#include "OrchestrionConfiguration/IOrchestrionConfiguration.h"
#include <QObject>
#include <actions/iactionsdispatcher.h>
#include <modularity/ioc.h>

namespace dgk
{
//! First-launch welcome card: shows until dismissed, and stays away for good
//! when dismissed with "don't show again" (persisted via configuration).
class FirstRunWelcomeModel : public QObject, public muse::Injectable
{
  Q_OBJECT

  Q_PROPERTY(bool active READ active NOTIFY activeChanged)

  muse::Inject<IOrchestrionConfiguration> configuration = {this};
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher = {this};

public:
  explicit FirstRunWelcomeModel(QObject *parent = nullptr);

  Q_INVOKABLE void init();
  Q_INVOKABLE void dismiss(bool dontShowAgain);

  //! Guides the tour's example-scores step: drops the File menu open (where
  //! "Example scores" lives) and closes it again when leaving the step.
  Q_INVOKABLE void openFileMenu();
  Q_INVOKABLE void closeAppMenu();

  bool active() const;

signals:
  void activeChanged();

private:
  void setActive(bool);

  bool m_active = false;
};
} // namespace dgk
