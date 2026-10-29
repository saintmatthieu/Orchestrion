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

#include "OrchestrionCommon/OrchestrionIoc.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/ITempoMultiplier.h"

#include <QObject>
#include <async/asyncable.h>
#include <context/iglobalcontext.h>

namespace dgk
{
/**
 * The tempo readout of the transport row: the tempo the score is played at
 * where the player is, in quarter notes per minute. Setting it scales the
 * whole score's tempo by the same ratio (see ITempoMultiplier), so that the
 * tempo changes the score marks keep their proportions.
 */
class TempoModel : public QObject,
                   public dgk::Injectable,
                   public muse::async::Asyncable
{
  Q_OBJECT

  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(int bpm READ bpm NOTIFY changed)
  Q_PROPERTY(int minBpm READ minBpm NOTIFY changed)
  Q_PROPERTY(int maxBpm READ maxBpm NOTIFY changed)
  Q_PROPERTY(double multiplier READ multiplier NOTIFY changed)

  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<ITempoMultiplier> tempoMultiplier{this};
  dgk::Inject<mu::context::IGlobalContext> globalContext{this};

public:
  explicit TempoModel(QObject *parent = nullptr);

  /** A score is open. */
  bool available() const;
  int bpm() const;
  /** The range setBpm() keeps to here. */
  int minBpm() const;
  int maxBpm() const;
  double multiplier() const;

  Q_INVOKABLE void load();
  /** Plays the score so that it is at `bpm` here. */
  Q_INVOKABLE void setBpm(int bpm);
  /** Back to the tempo as marked. */
  Q_INVOKABLE void reset();

signals:
  void changed();

private:
  void followSequencer();
  /** The score's tempo as marked here, in quarter notes per minute. */
  double markedBpm() const;

  /** With repeats: where the player is. */
  int m_tick = 0;
};
} // namespace dgk
