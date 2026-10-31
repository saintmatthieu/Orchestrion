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

#include "OrchestrionShell/ITutorials.h"
#include "async/asyncable.h"
#include "project/iprojectsession.h"
#include <QObject>
#include <QTimer>

#include "OrchestrionCommon/OrchestrionIoc.h"

namespace dgk
{
/**
 * Backs the "New here?" offer of the Getting started tutorial: shown a moment
 * after a score opens, until the user has played the tutorial from it or
 * closed it, which is remembered. Hidden while a tutorial runs.
 */
class TutorialOfferModel : public QObject,
                           public dgk::Injectable,
                           public muse::async::Asyncable
{
  Q_OBJECT

  Q_PROPERTY(bool shown READ shown NOTIFY shownChanged)

  dgk::Inject<mu::project::IProjectSession> projectSession{this};
  dgk::Inject<ITutorials> tutorials{this};

public:
  explicit TutorialOfferModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();
  /** Runs Getting started; the offer won't show again. */
  Q_INVOKABLE void play();
  /** The offer won't show again. */
  Q_INVOKABLE void dismiss();

  bool shown() const;

signals:
  void shownChanged();

private:
  void update();
  void setShown(bool);

  QTimer m_delay;
  bool m_dismissed = false;
  bool m_shown = false;
};
} // namespace dgk
