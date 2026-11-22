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
  bool IsReplaying() const override { return m_activeReplay != 0; }
  bool IsPlaying() const override { return m_playing; }
  muse::async::Notification PlayingChanged() const override
  {
    return m_playingChanged;
  }
  void
  FollowRecording(std::shared_ptr<const RecordingTimeline> timeline) override;
  void SetRecordingClock(double seconds,
                         std::chrono::steady_clock::time_point when,
                         double rate, bool playing) override;
  muse::async::Channel<double> RecordingSeekRequested() const override
  {
    return m_recordingSeekRequested;
  }
  bool IsFollowing() const override { return m_following; }

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
  void FireAndContinue(const NextAutoPlayEvents &events,
                       PlayThread::Clock::time_point due);
  double TicksToMilliseconds(int ticks) const;
  double ElapsedMs() const;
  void StartReplay();
  void ScheduleReplayNext();
  void FireReplayEvent();
  /** Has the main thread stop playing, as the stop action does. */
  void DispatchStop();
  /** Whether to follow the recording, as the main thread last decided. */
  void UpdateFollowing();
  void SetFollowActive(bool active);
  void ScheduleFollowNext();
  void FireFollowed();
  /** The recording's seconds now, by its clock. */
  double RecordingSecondsNow() const;
  /** When the recording reaches `seconds`, by its clock. */
  PlayThread::Clock::time_point RecordingTime(double seconds) const;
  /** The user moved the score while it followed: the recording follows. */
  void RequestRecordingSeek();

  const std::shared_ptr<OrchestrionSequencer> m_sequencer;
  const std::shared_ptr<PlayThread> m_thread;

  // Main thread's, read on any
  std::atomic<bool> m_playing = false;
  muse::async::Notification m_playingChanged;
  muse::async::Channel<double> m_recordingSeekRequested;
  // Following a recording (see FollowRecording): its timeline; whether the
  // player follows it, which counts as playing; and whether a stop asked
  // while it played holds until the recording stops, as its clock is still
  // told playing until it has been paused.
  std::shared_ptr<const RecordingTimeline> m_followTimeline;
  bool m_following = false;
  bool m_followStopped = false;
  /**
   * The replay under way, by its serial number; 0 if none. The play thread
   * starts it, the main thread ends it — after the releases the stop sends,
   * so that those aren't recorded as take events — and only that one: by
   * then, another may have started.
   */
  std::atomic<unsigned> m_activeReplay = 0;
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
  // Following a recording: its timeline and clock as last told, whether to
  // follow it, whether the score is following it, and where: the step fired
  // last and the step scheduled next.
  struct RecordingClock
  {
    PlayThread::Clock::time_point when;
    double seconds = 0.0;
    double rate = 1.0;
  };
  std::shared_ptr<const RecordingTimeline> m_timeline;
  RecordingClock m_clock;
  bool m_followActive = false;
  bool m_followRunning = false;
  int m_followTick = 0;
  std::optional<NextAutoPlayEvents> m_followNext;
  /**
   * Where the recording was asked to go, until its clock says it is there:
   * the clock told meanwhile still says where it was.
   */
  struct RecordingSeek
  {
    double seconds = 0.0;
    PlayThread::Clock::time_point until;
  };
  std::optional<RecordingSeek> m_recordingSeek;
  std::size_t m_replayIndex = 0;
  unsigned m_replaySerial = 0;
  PlayThread::Clock::time_point m_replayClockStart;
};
} // namespace dgk
