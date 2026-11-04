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

#include "OrchestrionShell/internal/PerformanceEndWatcher.h"
#include "async/asyncable.h"
#include <QObject>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
/**
 * Backs the hint that there are more scores than the one open: shown a few
 * seconds after the performer's own playing has reached the end of a score
 * (not the automatic player's, nor the "Show me!" demo's), until the user has
 * found the score shelf — by the hint, or any other way. The next note hides
 * it until the next end; closing it, or opening the shelf, puts it away for
 * good.
 */
class ScoreShelfHintModel : public QObject,
                            public dgk::Injectable,
                            public muse::async::Asyncable
{
  Q_OBJECT

  Q_PROPERTY(bool shown READ shown NOTIFY shownChanged)

public:
  explicit ScoreShelfHintModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();
  /**
   * The user has found the shelf, or doesn't want to hear of it: the hint
   * won't show again.
   */
  Q_INVOKABLE void dismiss();

  bool shown() const;

signals:
  void shownChanged();

private:
  void setShown(bool);

  bool m_dismissed = false;
  bool m_shown = false;
  PerformanceEndWatcher m_end;
};
} // namespace dgk
