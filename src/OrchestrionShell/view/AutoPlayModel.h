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

#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#include "async/asyncable.h"
#include "modularity/ioc.h"
#include <QObject>

namespace dgk
{
//! Backs the auto-play UI: the top-row button and its choice popup, plus the
//! Auto-play menu. Which hand the machine plays, following the performer's
//! tempo — see IOrchestrionSequencerConfiguration::autoPlayedStaff().
class AutoPlayModel : public QObject,
                      public muse::async::Asyncable,
                      public muse::actions::Actionable
{
  Q_OBJECT

  //! −1 = off, 0 = the machine plays the right hand, 1 = the left.
  Q_PROPERTY(int autoPlayedStaff READ autoPlayedStaff WRITE setAutoPlayedStaff
                 NOTIFY autoPlayedStaffChanged)
  Q_PROPERTY(bool exposed READ exposed NOTIFY exposedChanged)

  INJECT(IOrchestrionSequencerConfiguration, sequencerConfiguration);
  INJECT(muse::actions::IActionsDispatcher, dispatcher);

public:
  explicit AutoPlayModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();

  int autoPlayedStaff() const;
  void setAutoPlayedStaff(int);
  bool exposed() const;

signals:
  void autoPlayedStaffChanged();
  void exposedChanged();
  //! The Auto-play menu's "Choose…" action fired: the view should open the
  //! choice popup.
  void openSettingsRequested();
};
} // namespace dgk
