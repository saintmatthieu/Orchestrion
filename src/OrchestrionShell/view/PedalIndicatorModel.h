/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "async/asyncable.h"
#include "modularity/ioc.h"
#include <QObject>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
/**
 * Backs the sustain-pedal indicator at the bottom of the score view: whether
 * the pedal is down right now, and whether the indicator is shown at all
 * (toggled from the View menu). Follows the pedal events the sequencer sends
 * to the synthesizer, so it shows exactly what sounds — the re-pedal's
 * lift-and-press included.
 */
class PedalIndicatorModel : public QObject,
                            public dgk::Injectable,
                            public muse::async::Asyncable
{
  Q_OBJECT

  Q_PROPERTY(bool pedalDown READ pedalDown NOTIFY pedalDownChanged)
  Q_PROPERTY(bool iconVisible READ iconVisible NOTIFY iconVisibleChanged)

  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<IOrchestrionSequencerConfiguration> sequencerConfiguration{this};

public:
  explicit PedalIndicatorModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();

  bool pedalDown() const;
  bool iconVisible() const;

signals:
  void pedalDownChanged();
  void iconVisibleChanged();

private:
  void followSequencer();
  void setPedalDown(bool);

  bool m_pedalDown = false;
};
} // namespace dgk
