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

#include "GestureControllers/IGestureInput.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "OrchestrionSequencer/IStaffModes.h"
#include "Take.h"

#include <QElapsedTimer>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <context/iglobalcontext.h>
#include <optional>

namespace dgk
{
class TakeRecorder;

/**
 * Replays a take (see Take) as it was played: sets the staff modes, the
 * ornament mode and the loop it was played with, goes to where it started,
 * then plays its events at their times — through the gesture controllers'
 * stream, as the performer's own.
 */
class TakeReplayer : public muse::async::Asyncable, public dgk::Injectable
{
  dgk::Inject<IGestureInput> gestureInput{this};
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<IOrchestrionSequencerConfiguration> sequencerConfiguration{this};
  dgk::Inject<IStaffModes> staffModes{this};
  dgk::Inject<mu::context::IGlobalContext> globalContext{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};

public:
  /** `recorder` is paused while a take is replayed. */
  explicit TakeReplayer(TakeRecorder &recorder);
  void init();

  /**
   * Replays `take` in the score open — once it is ready, if it is being
   * opened.
   */
  void Replay(Take take);
  void Stop();

private:
  void Start();
  void ScheduleNext();
  void Fire();

  TakeRecorder &m_recorder;
  std::optional<Take> m_take;
  size_t m_index = 0;
  QElapsedTimer m_clock;
  /** Counts the replays, so that a stopped one's timers do nothing. */
  unsigned m_generation = 0;
};
} // namespace dgk
