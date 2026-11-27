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
#include "AutomaticOrchestrionPlayer.h"
#include "MuseScoreShell/OrchestrionActionIds.h"
#include "OrchestrionCommon/LatencyTrace.h"
#include <QMetaObject>
#include <algorithm>
#include <cmath>
#include <log.h>

namespace dgk
{
namespace
{
constexpr int ticksPerQuarterNote = 480;
/**
 * Following a recording, the score counts as in step with it while the
 * recording is between the step fired last and the one due next, give or
 * take this: the clock it is told jitters a little.
 */
constexpr double kFollowLeewaySeconds = 0.3;
/**
 * The recording asked to go somewhere is taken to be there once its clock
 * says it is this close, and to have refused if it doesn't within the time.
 */
constexpr double kSeekReachedSeconds = 1.0;
constexpr std::chrono::seconds kSeekTimeout{2};
/** A paused recording moved this far has been moved (see
 * PlaceWhereRecordingIs). */
constexpr double kPlacedSeconds = 0.25;
} // namespace

AutomaticOrchestrionPlayer::AutomaticOrchestrionPlayer(
    std::shared_ptr<OrchestrionSequencer> sequencer,
    std::shared_ptr<PlayThread> thread)
    : m_sequencer{std::move(sequencer)}, m_thread{std::move(thread)}
{
  m_thread->Wait(
      [this]
      {
        m_playThreadSubscriptions = std::make_unique<muse::async::Asyncable>();
        m_sequencer->AboutToJumpPosition().onReceive(
            m_playThreadSubscriptions.get(),
            [this](int /*tick*/, JumpReason reason)
            {
              ++m_generation;
              if (reason == JumpReason::Replay ||
                  reason == JumpReason::Recording)
                return; // our own rewind to the take's start, or following
              if (m_followRunning)
              {
                m_followNext.reset();
                // The score's end isn't where the recording goes on: it
                // ends there. A staff mode chosen leaves the score where it
                // was. Otherwise, wherever the score went, the recording
                // goes too, once the jump is done.
                if (reason == JumpReason::Realign)
                  Post(
                      [this]
                      {
                        m_followTick = m_sequencer->AutoPlayTick();
                        ScheduleFollowNext();
                      });
                else if (reason != JumpReason::ScoreEnd)
                  Post([this] { RequestRecordingSeek(); });
                return;
              }
              if (IsReplaying())
              {
                // The user navigated away mid-replay: end it.
                DispatchStop();
                return;
              }
              if (m_running && !m_firingInputEvents)
              {
                // The user moved the position: the schedule restarts from
                // now.
                m_nextMs = ElapsedMs();
                ScheduleNext();
              }
            });
      });

  // The tempo is read on the main thread, and told to the play thread.
  PostTempo();
  playbackController()->currentTempoChanged().onNotify(this,
                                                       [this] { PostTempo(); });

  // Orchestrion owns its playing state, driven by its own transport actions
  // (see OrchestrionActionIds.h). MuseScore's "play"/"stop" are neither
  // handled nor dispatched: its transport machinery was built for
  // rendered-track playback — a playhead running over the (now silent)
  // tracks, auto-stopping at *its* score end, stopped by *its* code (e.g.
  // the preferences dialog) — while Orchestrion playback is just scheduled
  // gesture events. With nothing dispatching MuseScore's ids, its transport
  // simply never runs.
  dispatcher()->reg(this, actionIds::playbackToggle, [this] { TogglePlay(); });
  dispatcher()->reg(this, actionIds::playbackStop, [this] { Stop(); });
}

AutomaticOrchestrionPlayer::~AutomaticOrchestrionPlayer()
{
  // Its tasks still posted are dropped, and the subscription goes where it
  // was made.
  m_thread->Wait(
      [this]
      {
        m_alive.reset();
        m_playThreadSubscriptions.reset();
      });
}

void AutomaticOrchestrionPlayer::Post(
    std::function<void()> task,
    std::optional<PlayThread::Clock::time_point> time)
{
  auto guarded = [alive = std::weak_ptr<int>{m_alive}, task = std::move(task)]
  {
    if (!alive.expired())
      task();
  };
  if (time)
    m_thread->PostAt(*time, std::move(guarded));
  else
    m_thread->Post(std::move(guarded));
}

void AutomaticOrchestrionPlayer::PostTempo()
{
  const double bpm = playbackController()->currentTempo().valueBpm;
  const double multiplier = playbackController()->tempoMultiplier();
  Post(
      [this, quartersPerMinute = bpm * multiplier, multiplier]
      {
        m_quartersPerMinute = quartersPerMinute;
        m_tempoMultiplier = multiplier > 0.0 ? multiplier : 1.0;
      });
}

void AutomaticOrchestrionPlayer::TogglePlay()
{
  if (m_playing)
  {
    Stop();
    return;
  }
  m_playing = true;
  PostTempo();
  Post([this] { Start(); });
  m_playingChanged.notify();
}

void AutomaticOrchestrionPlayer::Stop()
{
  if (m_following)
  {
    // Until the recording is paused, its clock still says it plays.
    m_followStopped = true;
    m_following = false;
    m_playing = false;
    Post([this] { SetFollowActive(false); });
    m_playingChanged.notify();
    return;
  }
  if (!m_playing)
  {
    // Not the player playing, then, but the performer: stopping is for them
    // to ask too. What sounds is let go of, and the performance's tempo
    // estimate with it, so that the Follow staves stop rather than play on,
    // gliding on the last tempo.
    Post([this] { m_sequencer->AllNotesOff(); });
    return;
  }
  m_playing = false;
  Post([this] { Halt(); });
  m_playingChanged.notify();
}

void AutomaticOrchestrionPlayer::Start()
{
  m_running = true;
  ++m_generation;
  if (m_replayTake)
    StartReplay();
  else
  {
    m_clockStart = PlayThread::Clock::now();
    m_nextMs = 0.0;
    ScheduleNext();
  }
}

void AutomaticOrchestrionPlayer::Halt()
{
  ++m_generation; // cancels all scheduled events, nominal and replay alike
  m_running = false;
  // Don't leave notes ringing. m_running is already false, so a position
  // jump this may cause cannot re-enter here; the replay still counts as
  // under way, so a mid-replay stop's releases aren't recorded as take
  // events: it ends on the main thread, once they are there.
  m_sequencer->AllNotesOff();
  if (const unsigned replay = m_activeReplay)
    QMetaObject::invokeMethod(
        &m_mainThreadContext,
        [this, replay]
        {
          unsigned expected = replay;
          m_activeReplay.compare_exchange_strong(expected, 0);
        },
        Qt::QueuedConnection);
}

void AutomaticOrchestrionPlayer::DispatchStop()
{
  QMetaObject::invokeMethod(
      &m_mainThreadContext,
      [this] { dispatcher()->dispatch(actionIds::playbackStop); },
      Qt::QueuedConnection);
}

void AutomaticOrchestrionPlayer::SetReplayTake(std::optional<ReplayTake> take)
{
  if (!take)
    m_activeReplay = 0;
  Post([this, take = std::move(take)]() mutable
       { m_replayTake = std::move(take); });
}

void AutomaticOrchestrionPlayer::StartReplay()
{
  m_activeReplay = ++m_replaySerial;
  m_replayIndex = 0;
  m_sequencer->GoToTick(m_replayTake->startTick, JumpReason::Replay);
  m_replayClockStart = PlayThread::Clock::now();
  ScheduleReplayNext();
}

void AutomaticOrchestrionPlayer::ScheduleReplayNext()
{
  if (m_replayIndex >= m_replayTake->events.size())
  {
    // Our own stop handler does the bookkeeping.
    DispatchStop();
    return;
  }

  // Schedule against the replay's absolute clock, not event-to-event deltas,
  // so timer latency doesn't accumulate: the whole point of the replay is
  // letting the user judge the performance's timing by ear.
  const auto due =
      m_replayClockStart +
      std::chrono::milliseconds{m_replayTake->events[m_replayIndex].ms};
  if (due > PlayThread::Clock::now())
  {
    const int gen = m_generation;
    Post(
        [this, gen]
        {
          if (gen == m_generation)
            FireReplayEvent();
        },
        due);
  }
  else
    FireReplayEvent();
}

void AutomaticOrchestrionPlayer::FireReplayEvent()
{
  if (!IsReplaying())
    return;
  const ReplayEvent &event = m_replayTake->events[m_replayIndex];
  m_sequencer->OnHandInputEvent(event.type, event.isLeftHand, event.velocity);
  ++m_replayIndex;
  ScheduleReplayNext();
}

void AutomaticOrchestrionPlayer::ScheduleNext()
{
  if (m_followRunning)
  {
    ScheduleFollowNext();
    return;
  }
  if (!m_running)
    return;

  const auto next = m_sequencer->WhatToPlayNext();
  if (!next)
  {
    DispatchStop();
    return;
  }
  // Events due at once are fired in a row, each calling this again: a
  // sequencer that never moves on would overflow the stack.
  m_firedInARow = next->deltaTicks > 0 ? 0 : m_firedInARow + 1;
  if (m_firedInARow > kMaxFiredInARow)
  {
    LOGE() << "The score doesn't move on at tick " << next->tick
           << ": playing stops";
    m_firedInARow = 0;
    DispatchStop();
    return;
  }

  if (next->deltaTicks > 0)
  {
    // The due time is accumulated on the play clock and the task is posted
    // for it, not for the tick delta: a task never runs early and the fired
    // events are handled before the next one is posted, so delta-to-delta
    // scheduling lost a few ms per event for good — about a 32nd per ten
    // bars. (The replay path has always done this, see ScheduleReplayNext.)
    // After a stall the overdue events catch up.
    m_nextMs += StepMilliseconds(*next);
    const auto due =
        m_clockStart + std::chrono::duration_cast<PlayThread::Clock::duration>(
                           std::chrono::duration<double, std::milli>{m_nextMs});
    const int gen = m_generation;
    Post(
        [this, events = *next, gen, due]
        {
          if (gen == m_generation)
            FireAndContinue(events, due);
        },
        due);
  }
  else
    FireAndContinue(*next, PlayThread::Clock::now());
}

void AutomaticOrchestrionPlayer::FireAndContinue(
    const NextAutoPlayEvents &events, PlayThread::Clock::time_point due)
{
  if (!m_running && !m_followRunning)
    return;
  LatencyTrace *const trace = LatencyTrace::instance();
  // Each note traced from when it was due, the notes the sequencer queues
  // meanwhile its own
  const auto traced = [&](NoteEventType type, int pitch, auto &&play)
  {
    std::uint32_t id = 0;
    if (trace)
    {
      id = trace->newNote(type == NoteEventType::noteOn, pitch,
                          LatencyTrace::toNs(due),
                          LatencyTrace::Source::automatic);
      trace->stamp(id, LatencyTrace::played);
    }
    const LatencyTrace::CurrentScope traceScope{id};
    play();
  };
  m_firingInputEvents = true;
  // Playing a performance, as fast as it goes: its ornaments too.
  if (m_running && m_tempoCurve)
    m_sequencer->SetPerformanceRate(m_tempoMultiplier);
  if (events.leftHandEvent)
    traced(*events.leftHandEvent, -1,
           [&] {
             m_sequencer->OnHandInputEvent(*events.leftHandEvent, true,
                                           std::nullopt);
           });
  if (events.rightHandEvent)
    traced(*events.rightHandEvent, -2,
           [&]
           {
             m_sequencer->OnHandInputEvent(*events.rightHandEvent, false,
                                           std::nullopt);
           });
  for (const auto &[type, pitch] : events.notes)
    traced(type, pitch,
           [&] { m_sequencer->OnInputEvent(type, pitch, std::nullopt); });
  if (events.pedal)
    m_sequencer->OnPerformedPedal(*events.pedal);
  m_sequencer->SetPerformanceRate(std::nullopt);
  m_firingInputEvents = false;
  ScheduleNext();
}

void AutomaticOrchestrionPlayer::SetTempoCurve(
    std::shared_ptr<const RecordingTimeline> timeline)
{
  if (timeline && timeline->Empty())
    timeline.reset();
  Post([this, timeline] { m_tempoCurve = timeline; });
}

double AutomaticOrchestrionPlayer::StepMilliseconds(
    const NextAutoPlayEvents &next) const
{
  if (m_tempoCurve && next.deltaTicks > 0)
  {
    // From where the sequencer was (wherever playing started, or jumped
    // to) to the step: as long as the recording takes...
    const int from = next.tick - next.deltaTicks;
    const auto start = m_tempoCurve->SecondsAt(from);
    const auto end = m_tempoCurve->SecondsAt(next.tick);
    if (start && end && *end > *start)
      return (*end - *start) * 1000.0 / m_tempoMultiplier;
    // ...or, where it doesn't play the score (before its first anchor, past
    // its last, a part it skips), at its pace thereabouts.
    if (const auto pace = m_tempoCurve->SecondsPerTick(next.tick))
      return next.deltaTicks * *pace * 1000.0 / m_tempoMultiplier;
  }
  return TicksToMilliseconds(next.deltaTicks);
}

double AutomaticOrchestrionPlayer::TicksToMilliseconds(int ticks) const
{
  if (m_quartersPerMinute <= 0)
    return 0.0;
  return ticks * 60000.0 / (m_quartersPerMinute * ticksPerQuarterNote);
}

double AutomaticOrchestrionPlayer::ElapsedMs() const
{
  return std::chrono::duration<double, std::milli>(PlayThread::Clock::now() -
                                                   m_clockStart)
      .count();
}
void AutomaticOrchestrionPlayer::FollowRecording(
    std::shared_ptr<const RecordingTimeline> timeline)
{
  m_followTimeline = timeline;
  Post(
      [this, timeline]
      {
        m_timeline = timeline;
        m_placedSeconds.reset();
      });
  if (!timeline && m_following)
  {
    m_following = false;
    m_playing = false;
    Post([this] { SetFollowActive(false); });
    m_playingChanged.notify();
  }
}

void AutomaticOrchestrionPlayer::SetRecordingClock(
    double seconds, std::chrono::steady_clock::time_point when, double rate,
    bool playing)
{
  if (!playing)
    m_followStopped = false;
  // Not over the score's own playing: it stops the recording.
  const bool follow = playing && m_followTimeline &&
                      !m_followTimeline->Empty() && !m_followStopped &&
                      (!m_playing || m_following);
  Post(
      [this, clock = RecordingClock{when, seconds, rate > 0.0 ? rate : 1.0},
       follow]
      {
        m_clock = clock;
        SetFollowActive(follow);
      });
  if (follow != m_following)
  {
    m_following = follow;
    m_playing = follow;
    m_playingChanged.notify();
  }
}

void AutomaticOrchestrionPlayer::SetFollowActive(bool active)
{
  m_followActive = active && m_timeline && !m_timeline->Empty();
  // Where it is, as it plays and when first told: once paused, a change is
  // the recording moved.
  if (m_timeline && (!m_placedSeconds || m_followActive))
    m_placedSeconds = m_clock.seconds;
  if (!m_followActive)
    m_recordingSeek.reset();
  UpdateFollowing();
}

void AutomaticOrchestrionPlayer::UpdateFollowing()
{
  // The schedule (of the score's own playing too) is only touched when
  // following, or to stop: the clock is told a few times a second.
  if (!m_followActive || m_running)
  {
    if (!m_followRunning && !m_running)
      PlaceWhereRecordingIs();
    if (m_followRunning)
    {
      ++m_generation;
      m_followRunning = false;
      m_followNext.reset();
      m_sequencer->AllNotesOff();
      m_sequencer->SetSilent(false);
    }
    return;
  }

  ++m_generation;
  const double now = RecordingSecondsNow();
  if (m_recordingSeek)
  {
    if (std::abs(now - m_recordingSeek->seconds) < kSeekReachedSeconds ||
        PlayThread::Clock::now() > m_recordingSeek->until)
      m_recordingSeek.reset();
    else
      return; // The score waits where it was moved to.
  }
  const auto position = m_timeline->PositionAt(now);
  if (!position)
  {
    // Before the recording's first beat: back then.
    const int gen = m_generation;
    Post(
        [this, gen]
        {
          if (gen == m_generation)
            UpdateFollowing();
        },
        RecordingTime(m_timeline->Beats().front().seconds));
    return;
  }

  bool inStep = false;
  if (!m_followRunning)
  {
    m_followRunning = true;
    m_sequencer->SetSilent(true);
    m_followNext.reset();
  }
  else if (const auto last = m_timeline->SecondsAt(m_followTick))
  {
    const auto next =
        m_followNext ? m_timeline->SecondsAt(m_followNext->tick) : std::nullopt;
    inStep = *last - kFollowLeewaySeconds <= now &&
             (!next || now <= *next + kFollowLeewaySeconds);
  }
  if (!inStep)
  {
    // Started, or sought: to where the recording is.
    m_followNext.reset();
    m_sequencer->GoToTickWithRepeats(position->tick, position->writtenTick,
                                     JumpReason::Recording);
    m_followTick = position->tick;
  }
  ScheduleFollowNext();
}

void AutomaticOrchestrionPlayer::ScheduleFollowNext()
{
  if (!m_followRunning)
    return;
  if (!m_followNext)
  {
    m_followNext = m_sequencer->WhatToPlayNext();
    if (!m_followNext)
      return; // The score is over; the recording may go on.
  }
  const int gen = m_generation;
  if (const auto seconds = m_timeline->SecondsAt(m_followNext->tick))
  {
    Post(
        [this, gen]
        {
          if (gen == m_generation)
            FireFollowed();
        },
        RecordingTime(*seconds));
    return;
  }

  // A part the recording skips (a repeat), or past where it ends.
  const RecordingBeat *next = m_timeline->NextPlayed(m_followNext->tick);
  if (!next)
  {
    m_followNext.reset();
    return;
  }
  if (next->tick <= m_followTick)
  {
    // Already there, say a release left behind by the skip: now.
    Post(
        [this, gen]
        {
          if (gen == m_generation)
            FireFollowed();
        });
    return;
  }
  Post(
      [this, gen, tick = next->tick, writtenTick = next->writtenTick]
      {
        if (gen != m_generation)
          return;
        m_followNext.reset();
        m_sequencer->GoToTickWithRepeats(tick, writtenTick,
                                         JumpReason::Recording);
        m_followTick = tick;
        ScheduleFollowNext();
      },
      RecordingTime(next->seconds));
}

void AutomaticOrchestrionPlayer::FireFollowed()
{
  if (!m_followNext)
    return;
  const NextAutoPlayEvents events = std::move(*m_followNext);
  m_followNext.reset();
  m_followTick = events.tick;
  FireAndContinue(events, PlayThread::Clock::now());
}

double AutomaticOrchestrionPlayer::RecordingSecondsNow() const
{
  const double elapsed =
      std::chrono::duration<double>(PlayThread::Clock::now() - m_clock.when)
          .count();
  return m_clock.seconds + elapsed * m_clock.rate;
}

PlayThread::Clock::time_point
AutomaticOrchestrionPlayer::RecordingTime(double seconds) const
{
  return m_clock.when + std::chrono::duration_cast<PlayThread::Clock::duration>(
                            std::chrono::duration<double>(
                                (seconds - m_clock.seconds) / m_clock.rate));
}

void AutomaticOrchestrionPlayer::RequestRecordingSeek()
{
  ++m_generation;
  m_followNext.reset();
  m_followTick = m_sequencer->AutoPlayTick();
  std::optional<double> seconds = m_timeline->SecondsAt(m_followTick);
  if (!seconds)
    if (const RecordingBeat *next = m_timeline->NextPlayed(m_followTick))
      seconds = next->seconds;
  if (!seconds)
    return;
  m_recordingSeek =
      RecordingSeek{*seconds, PlayThread::Clock::now() + kSeekTimeout};
  QMetaObject::invokeMethod(
      &m_mainThreadContext, [this, seconds = *seconds]
      { m_recordingSeekRequested.send(seconds); }, Qt::QueuedConnection);
}
void AutomaticOrchestrionPlayer::PlaceWhereRecordingIs()
{
  // Paused, and moved (a click on its progress bar): the score goes there
  // too. Only once it has been followed, and when it moves.
  if (!m_timeline || !m_placedSeconds)
    return;
  const double now = m_clock.seconds;
  if (std::abs(now - *m_placedSeconds) < kPlacedSeconds)
    return;
  m_placedSeconds = now;
  if (const auto position = m_timeline->PositionAt(now))
  {
    m_sequencer->GoToTickWithRepeats(position->tick, position->writtenTick,
                                     JumpReason::Recording);
  }
}
} // namespace dgk
