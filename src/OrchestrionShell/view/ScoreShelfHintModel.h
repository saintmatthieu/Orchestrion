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
#include "async/asyncable.h"
#include "modularity/ioc.h"
#include <QObject>
#include <QTimer>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
/**
 * Backs the card that comes up a few seconds after the performer's own
 * playing has reached the end of a score (not the automatic player's, nor the
 * "Show me!" demo's): the next score, and the way to all of them — with, until
 * the user has found the score shelf (by the card, or any other way), a word
 * on how to get there. The next note hides it until the next end, and so do
 * closing it and opening the shelf, which also tell that the shelf is found.
 */
class ScoreShelfHintModel : public QObject,
                            public dgk::Injectable,
                            public muse::async::Asyncable
{
  Q_OBJECT

  /** Whether the end was reached, a moment ago, and not played on since. */
  Q_PROPERTY(bool shown READ shown NOTIFY shownChanged)
  /** Whether the user has found the score shelf: no need to tell the way. */
  Q_PROPERTY(bool shelfKnown READ shelfKnown NOTIFY shelfKnownChanged)

  dgk::Inject<IOrchestrion> orchestrion{this};

public:
  explicit ScoreShelfHintModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();
  /**
   * The card was closed or used, or the shelf opened: it hides until the next
   * end, and won't tell the way to the shelf again.
   */
  Q_INVOKABLE void dismiss();

  bool shown() const;
  bool shelfKnown() const;

signals:
  void shownChanged();
  void shelfKnownChanged();

private:
  void followSequencer();
  void setShown(bool);

  bool m_shelfKnown = false;
  bool m_shown = false;
  /** Whether the last note struck was the performer's. */
  bool m_performerStruck = false;
  /** From the end of the piece to the card. */
  QTimer m_delay;
};
} // namespace dgk
