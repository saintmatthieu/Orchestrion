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

#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#include "async/asyncable.h"
#include "modularity/ioc.h"
#include <QObject>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
//! Backs the grading UI: the top-row toggle button, the grading settings
//! dialog (the master switch plus its dependent settings), and the Grading
//! menu's "Settings…" action, which requests the dialog through
//! openSettingsRequested().
class GradingModel : public QObject,
                     public dgk::Injectable,
                     public muse::async::Asyncable,
                     public muse::actions::Actionable
{
  Q_OBJECT

  Q_PROPERTY(bool gradingEnabled READ gradingEnabled WRITE setGradingEnabled
                 NOTIFY gradingEnabledChanged)
  Q_PROPERTY(bool exposed READ exposed NOTIFY exposedChanged)
  Q_PROPERTY(bool persistentMarks READ persistentMarks WRITE setPersistentMarks
                 NOTIFY persistentMarksChanged)
  Q_PROPERTY(bool handSyncScore READ handSyncScore WRITE setHandSyncScore NOTIFY
                 handSyncScoreChanged)
  Q_PROPERTY(bool dynamicsScore READ dynamicsScore WRITE setDynamicsScore NOTIFY
                 dynamicsScoreChanged)
  Q_PROPERTY(bool proportionalSpacing READ proportionalSpacing WRITE
                 setProportionalSpacing NOTIFY proportionalSpacingChanged)
  //! 0 = replay performance, 1 = replay at fitted tempo, 2 = metronome
  //! (mirrors dgk::PlayMode).
  Q_PROPERTY(
      int playMode READ playMode WRITE setPlayMode NOTIFY playModeChanged)

  dgk::Inject<IOrchestrionSequencerConfiguration> sequencerConfiguration{this};
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};

public:
  explicit GradingModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();

  bool gradingEnabled() const;
  void setGradingEnabled(bool);
  bool exposed() const;
  bool persistentMarks() const;
  void setPersistentMarks(bool);
  bool handSyncScore() const;
  void setHandSyncScore(bool);
  bool dynamicsScore() const;
  void setDynamicsScore(bool);
  bool proportionalSpacing() const;
  void setProportionalSpacing(bool);
  int playMode() const;
  void setPlayMode(int);

signals:
  void gradingEnabledChanged();
  void exposedChanged();
  void persistentMarksChanged();
  void handSyncScoreChanged();
  void dynamicsScoreChanged();
  void proportionalSpacingChanged();
  void playModeChanged();
  //! The Grading menu's "Settings…" action fired: the view should open the
  //! settings dialog.
  void openSettingsRequested();
};
} // namespace dgk
