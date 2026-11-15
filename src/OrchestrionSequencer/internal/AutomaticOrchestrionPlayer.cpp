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
#include <QMetaObject>
#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
constexpr int ticksPerQuarterNote = 480;
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
              if (reason == JumpReason::Replay)
                return; // our own rewind to the take's start
              if (m_replayActive)
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
  Post([this, quartersPerMinute = bpm * multiplier]
       { m_quartersPerMinute = quartersPerMinute; });
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
  // jump this may cause cannot re-enter here; m_replayActive is still true,
  // so a mid-replay stop's releases aren't recorded as take events: it is
  // reset on the main thread, once they are there.
  m_sequencer->AllNotesOff();
  QMetaObject::invokeMethod(
      &m_mainThreadContext, [this] { m_replayActive = false; },
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
    m_replayActive = false;
  Post([this, take = std::move(take)]() mutable
       { m_replayTake = std::move(take); });
}

void AutomaticOrchestrionPlayer::StartReplay()
{
  m_replayActive = true;
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
  if (!m_replayActive)
    return;
  const ReplayEvent &event = m_replayTake->events[m_replayIndex];
  m_sequencer->OnHandInputEvent(event.type, event.isLeftHand, event.velocity);
  ++m_replayIndex;
  ScheduleReplayNext();
}

void AutomaticOrchestrionPlayer::ScheduleNext()
{
  if (!m_running)
    return;

  const auto next = m_sequencer->WhatToPlayNext();
  if (!next)
  {
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
    m_nextMs += TicksToMilliseconds(next->deltaTicks);
    const auto due =
        m_clockStart + std::chrono::duration_cast<PlayThread::Clock::duration>(
                           std::chrono::duration<double, std::milli>{m_nextMs});
    const int gen = m_generation;
    Post(
        [this, events = *next, gen]
        {
          if (gen == m_generation)
            FireAndContinue(events);
        },
        due);
  }
  else
    FireAndContinue(*next);
}

void AutomaticOrchestrionPlayer::FireAndContinue(
    const NextAutoPlayEvents &events)
{
  if (!m_running)
    return;
  m_firingInputEvents = true;
  if (events.leftHandEvent)
    m_sequencer->OnHandInputEvent(*events.leftHandEvent, true, std::nullopt);
  if (events.rightHandEvent)
    m_sequencer->OnHandInputEvent(*events.rightHandEvent, false, std::nullopt);
  for (const auto &[type, pitch] : events.notes)
    m_sequencer->OnInputEvent(type, pitch, std::nullopt);
  m_firingInputEvents = false;
  ScheduleNext();
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
} // namespace dgk
