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

#include "IOrchestrionPlayer.h"
#include "internal/OrchestrionSequencer.h"
#include "internal/PlayThread.h"
#include <QObject>
#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <async/notification.h>
#include <atomic>
#include <memory>
#include <modularity/ioc.h>
#include <playback/iplaybackcontroller.h>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
/**
 * Created and destroyed on the main thread, where its transport is (the
 * play/stop actions, IOrchestrionPlayer); it plays on the play thread (see
 * ../threading.md), straight to the sequencer. The members are the main
 * thread's or the play thread's, as grouped below.
 */
class AutomaticOrchestrionPlayer : public IOrchestrionPlayer,
                                   public muse::async::Asyncable,
                                   public muse::actions::Actionable,
                                   public dgk::Injectable
{
  dgk::Inject<mu::playback::IPlaybackController> playbackController{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};

public:
  AutomaticOrchestrionPlayer(std::shared_ptr<OrchestrionSequencer>,
                             std::shared_ptr<PlayThread>);
  ~AutomaticOrchestrionPlayer() override;

  // IOrchestrionPlayer
  void SetReplayTake(std::optional<ReplayTake> take) override;
  /** Any thread. */
  bool IsReplaying() const override { return m_replayActive; }
  bool IsPlaying() const override { return m_playing; }
  muse::async::Notification PlayingChanged() const override
  {
    return m_playingChanged;
  }

private:
  // Main thread
  void TogglePlay();
  void Stop();
  /** Tells the play thread the score's tempo. */
  void PostTempo();
  /** Runs the task on the play thread, unless the player is gone by then. */
  void Post(std::function<void()>,
            std::optional<PlayThread::Clock::time_point> time = std::nullopt);

  // Play thread
  void Start();
  void Halt();
  void ScheduleNext();
  void FireAndContinue(const NextAutoPlayEvents &events);
  double TicksToMilliseconds(int ticks) const;
  double ElapsedMs() const;
  void StartReplay();
  void ScheduleReplayNext();
  void FireReplayEvent();
  /** Has the main thread stop playing, as the stop action does. */
  void DispatchStop();

  const std::shared_ptr<OrchestrionSequencer> m_sequencer;
  const std::shared_ptr<PlayThread> m_thread;

  // Main thread's, read on any
  std::atomic<bool> m_playing = false;
  muse::async::Notification m_playingChanged;
  /**
   * The play thread starts it, the main thread stops it — after the
   * releases the stop sends, so that those aren't recorded as take events.
   */
  std::atomic<bool> m_replayActive = false;
  /** What the main thread is told things in, by the play thread. */
  QObject m_mainThreadContext;
  /**
   * Its tasks posted to the play thread are dropped once it is gone: let go
   * of there, as it is destroyed.
   */
  std::shared_ptr<int> m_alive = std::make_shared<int>(0);

  // Play thread's
  /** Its subscription to the sequencer, made and let go of there. */
  std::unique_ptr<muse::async::Asyncable> m_playThreadSubscriptions;
  bool m_running = false;
  // Used to cancel previously scheduled calls to FireAndContinue when the
  // position changes e.g. by the user pressing left/right during playback.
  int m_generation = 0;
  // True while our own OnInputEvent calls are on the stack. A position jump
  // they cause (e.g. the loop wrap-around) must not re-enter ScheduleNext
  // from the AboutToJumpPosition notification — the sequencer state is not
  // settled yet, and FireAndContinue reschedules after they return anyway.
  bool m_firingInputEvents = false;
  /** The score's tempo, quarter notes per minute times the multiplier. */
  double m_quartersPerMinute = 0.0;
  // The play clock, started with nominal playback, and the due time on it of
  // the events scheduled last: accumulated from the score's tick deltas, so
  // that timer latency doesn't build up into drift (see ScheduleNext).
  PlayThread::Clock::time_point m_clockStart;
  double m_nextMs = 0.0;
  std::optional<ReplayTake> m_replayTake;
  std::size_t m_replayIndex = 0;
  PlayThread::Clock::time_point m_replayClockStart;
};
} // namespace dgk
