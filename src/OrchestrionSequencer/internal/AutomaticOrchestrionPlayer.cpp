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
#include "OrchestrionCommon/PerfTrace.h"
#include <QTimer>
#include <cmath>

namespace dgk
{
namespace
{
constexpr int rightHandPitch = 60;
constexpr int leftHandPitch = 59;
constexpr int ticksPerQuarterNote = 480;
} // namespace

AutomaticOrchestrionPlayer::AutomaticOrchestrionPlayer(
    IOrchestrionSequencer &sequencer)
    : m_sequencer{sequencer}
{
  sequencer.AboutToJumpPosition().onReceive(
      this,
      [this](int /*tick*/)
      {
        ++m_generation;
        if (m_selfJump)
          return; // the replay's own rewind to the take's start
        if (m_replayActive)
        {
          // The user navigated away mid-replay: end it.
          dispatcher()->dispatch(actionIds::playbackStop);
          return;
        }
        if (!m_firingInputEvents)
          ScheduleNext();
      });

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

void AutomaticOrchestrionPlayer::TogglePlay()
{
  if (m_playing)
  {
    Stop();
    return;
  }
  m_playing = true;
  ++m_generation;
  m_playingChanged.notify();
  if (m_replayTake)
    StartReplay();
  else
    ScheduleNext();
}

void AutomaticOrchestrionPlayer::Stop()
{
  if (!m_playing)
    return;
  ++m_generation; // cancels all scheduled events, nominal and replay alike
  m_playing = false;
  m_replayActive = false;
  m_playingChanged.notify();
}

void AutomaticOrchestrionPlayer::SetReplayTake(std::optional<ReplayTake> take)
{
  m_replayTake = std::move(take);
  if (!m_replayTake)
    m_replayActive = false;
}

void AutomaticOrchestrionPlayer::StartReplay()
{
  m_replayActive = true;
  m_replayIndex = 0;
  m_selfJump = true;
  m_sequencer.GoToTick(m_replayTake->startTick);
  m_selfJump = false;
  m_replayClock.start();
  ScheduleReplayNext();
}

void AutomaticOrchestrionPlayer::ScheduleReplayNext()
{
  if (m_replayIndex >= m_replayTake->events.size())
  {
    // Our own stop handler does the bookkeeping.
    dispatcher()->dispatch(actionIds::playbackStop);
    return;
  }

  // Schedule against the replay's absolute clock, not event-to-event deltas,
  // so timer latency doesn't accumulate: the whole point of the replay is
  // letting the user judge the performance's timing by ear.
  const int delay = static_cast<int>(m_replayTake->events[m_replayIndex].ms -
                                     m_replayClock.elapsed());
  if (delay > 0)
  {
    const int gen = m_generation;
    const long long dueUs = PerfTrace::nowUs() + 1000LL * delay;
    QTimer::singleShot(delay, Qt::PreciseTimer,
                       [this, gen, dueUs]
                       {
                         // How late the GUI thread got round to this timer.
                         PerfTrace::event("gui", "replay_late",
                                          PerfTrace::nowUs() - dueUs);
                         if (gen == m_generation)
                           FireReplayEvent();
                       });
  }
  else
    FireReplayEvent();
}

void AutomaticOrchestrionPlayer::FireReplayEvent()
{
  if (!m_replayActive)
    return;
  const ReplayEvent &event = m_replayTake->events[m_replayIndex];
  m_sequencer.OnInputEvent(event.type,
                           event.isLeftHand ? leftHandPitch : rightHandPitch,
                           event.velocity);
  ++m_replayIndex;
  ScheduleReplayNext();
}

void AutomaticOrchestrionPlayer::ScheduleNext()
{
  if (!m_playing)
    return;

  const auto next = m_sequencer.WhatToPlayNext();
  if (!next)
  {
    dispatcher()->dispatch(actionIds::playbackStop);
    return;
  }

  if (next->deltaTicks > 0)
  {
    const int gen = m_generation;
    // Qt::PreciseTimer (not the default coarse, ~5%-accurate timer) so the
    // auto-played onsets land close to their intended times even while the UI
    // thread is busy painting; that arrival time is what the tempo model
    // timestamps, so timer jitter shows up as tempo dents.
    const int delayMs = TicksToMilliseconds(next->deltaTicks);
    const long long dueUs = PerfTrace::nowUs() + 1000LL * delayMs;
    QTimer::singleShot(delayMs, Qt::PreciseTimer,
                       [this, events = *next, gen, dueUs]
                       {
                         // How late the GUI thread got round to this timer:
                         // the onset's jitter, before anything downstream.
                         PerfTrace::event("gui", "autoplay_late",
                                          PerfTrace::nowUs() - dueUs);
                         if (gen == m_generation)
                           FireAndContinue(events);
                       });
  }
  else
    FireAndContinue(*next);
}

void AutomaticOrchestrionPlayer::FireAndContinue(
    const NextAutoPlayEvents &events)
{
  if (!m_playing)
    return;
  m_firingInputEvents = true;
  if (events.leftHandEvent)
    m_sequencer.OnInputEvent(*events.leftHandEvent, leftHandPitch,
                             std::nullopt);
  if (events.rightHandEvent)
    m_sequencer.OnInputEvent(*events.rightHandEvent, rightHandPitch,
                             std::nullopt);
  m_firingInputEvents = false;
  ScheduleNext();
}

int AutomaticOrchestrionPlayer::TicksToMilliseconds(int ticks) const
{
  const double bpm = playbackController()->currentTempo().valueBpm;
  const double multiplier = playbackController()->tempoMultiplier();
  if (bpm <= 0 || multiplier <= 0)
    return 0;
  // Round (not truncate) so the per-note delay doesn't bias short.
  return static_cast<int>(
      std::lround(ticks * 60000.0 / (bpm * ticksPerQuarterNote * multiplier)));
}
} // namespace dgk
