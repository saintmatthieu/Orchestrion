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
#include "async/notification.h"
#include "modularity/ioc.h"

#include <QTimer>

#include "OrchestrionCommon/OrchestrionIoc.h"

namespace dgk
{
/**
 * Tells when the performer's own playing has reached the end of a score (not
 * the automatic player's, nor a tutorial's), a few seconds after the last
 * notes were released, whether grading is on or not.
 */
class PerformanceEndWatcher : public dgk::Injectable,
                              public muse::async::Asyncable
{
  dgk::Inject<IOrchestrion> orchestrion{this};

public:
  PerformanceEndWatcher();

  void start();

  muse::async::Notification reachedEnd() const;
  /** The performer plays on, or another score was opened. */
  muse::async::Notification leftEnd() const;

private:
  void followSequencer();
  void onStruck();
  void onChordTransition();

  /** Whether the last note struck was the performer's. */
  bool m_performerStruck = false;
  /** From the last notes' release until the next note. */
  bool m_atEnd = false;
  QTimer m_delay;
  muse::async::Notification m_reachedEnd;
  muse::async::Notification m_leftEnd;
};
} // namespace dgk
