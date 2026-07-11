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
          m_replayActive = false;
          dispatcher()->dispatch("stop");
          return;
        }
        if (!m_firingInputEvents)
          ScheduleNext();
      });

  playbackController()->isPlayingChanged().onNotify(
      this,
      [this]
      {
        m_playing = playbackController()->isPlaying();
        if (m_playing)
        {
          ++m_generation;
          if (m_replayTake)
            StartReplay();
          else
            ScheduleNext();
        }
        else
          m_replayActive = false; // pause ends the replay; play restarts it
      });
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
    dispatcher()->dispatch("stop");
    m_playing = false;
    m_replayActive = false;
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
    QTimer::singleShot(delay, Qt::PreciseTimer,
                       [this, gen]
                       {
                         if (gen == m_generation)
                           FireReplayEvent();
                       });
  }
  else
    FireReplayEvent();
}

void AutomaticOrchestrionPlayer::FireReplayEvent()
{
  if (!m_playing || !m_replayActive)
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
    dispatcher()->dispatch("stop");
    m_playing = false;
    return;
  }

  if (next->deltaTicks > 0)
  {
    const int gen = m_generation;
    // Qt::PreciseTimer (not the default coarse, ~5%-accurate timer) so the
    // auto-played onsets land close to their intended times even while the UI
    // thread is busy painting; that arrival time is what the tempo model
    // timestamps, so timer jitter shows up as tempo dents.
    QTimer::singleShot(TicksToMilliseconds(next->deltaTicks), Qt::PreciseTimer,
                       [this, events = *next, gen]
                       {
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
