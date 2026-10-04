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
#include "FollowPlayer.h"
#include "IChord.h"

#include <algorithm>
#include <async/async.h>
#include <cmath>
#include <vector>

namespace dgk
{
FollowPlayer::FollowPlayer(IOrchestrionSequencer &sequencer,
                           const IOrchestrionPlayer &player)
    : m_sequencer{sequencer}, m_player{player}
{
  m_timer.setInterval(16); // ~60 Hz
  m_timer.callOnTimeout([this] { Poll(); });

  m_sequencer.ChordTransitions().onReceive(
      this, [this](const std::map<TrackIndex, ChordTransition> &transitions)
      { OnTransitions(transitions); });
  // The jump's transitions batch repopulates the ledger.
  m_sequencer.AboutToJumpPosition().onReceive(this, [this](int, JumpReason)
                                              { m_trackTargets.clear(); });
  staffModes()->modesChanged().onNotify(this, [this] { Reconfigure(); });
  Reconfigure();
}

bool FollowPlayer::IsFollowStaff(int staff) const
{
  return staffModes()->mode(staff) == StaffMode::follow;
}

void FollowPlayer::Reconfigure()
{
  m_trackTargets.clear();
  m_staffTargets.clear();
  m_timer.stop();
  OnTransitions(m_sequencer.GetCurrentTransitions());
}

void FollowPlayer::OnTransitions(
    const std::map<TrackIndex, ChordTransition> &batch)
{
  if (m_player.IsReplaying())
  {
    // The recorded take already contains the Follow staves' events: keep
    // quiet, so they don't sound twice.
    m_staffTargets.clear();
    m_timer.stop();
    return;
  }

  for (const auto &[track, transition] : batch)
  {
    if (!IsFollowStaff(track.staffIndex()))
      continue;
    Targets &targets = m_trackTargets[track.value];
    const IChord *present = GetPresentChord(transition);
    targets.offTick =
        present ? std::make_optional<double>(present->GetEndTick().withRepeats)
                : std::nullopt;
    const IChord *future = GetFutureChord(transition);
    if (!future && GetPresentThing(transition) && !present)
      // Released onto a rest, which the transition says nothing after: the
      // chord after it is what the staff strikes next.
      future = m_sequencer.GetNextChord(track);
    targets.onTick =
        future ? std::make_optional<double>(future->GetBeginTick().withRepeats)
               : std::nullopt;
  }
  Aggregate();
}

void FollowPlayer::Aggregate()
{
  m_staffTargets.clear();
  for (const auto &[track, targets] : m_trackTargets)
  {
    Targets &staff = m_staffTargets[TrackIndex{track}.staffIndex()];
    if (targets.offTick)
      staff.offTick = staff.offTick ? std::min(*staff.offTick, *targets.offTick)
                                    : targets.offTick;
    if (targets.onTick)
      staff.onTick = staff.onTick ? std::min(*staff.onTick, *targets.onTick)
                                  : targets.onTick;
  }
  // Poll only while there is something to fire.
  const bool due = std::any_of(
      m_staffTargets.begin(), m_staffTargets.end(), [](const auto &entry)
      { return entry.second.offTick || entry.second.onTick; });
  if (!due)
    m_timer.stop();
  else if (!m_timer.isActive())
    m_timer.start();
}

void FollowPlayer::Poll()
{
  const auto performance = m_sequencer.PerformedTempoNow();
  if (!performance)
  {
    m_awaitsTempo = true;
    return;
  }
  const double now = performance->tick;
  if (m_awaitsTempo)
  {
    m_awaitsTempo = false;
    if (Join(now))
      return;
  }

  // A target fired is forgotten in the ledger too, not only in the
  // aggregate: the next batch, of any staff, rebuilds the aggregate from the
  // ledger, and would bring it back to fire again. Only those due, though: a
  // voice struck later keeps its own.
  const auto forget = [this, now](int staff, bool onTick)
  {
    const auto due = [now](std::optional<double> &tick)
    {
      if (tick && *tick <= now)
        tick.reset();
    };
    for (auto &[track, targets] : m_trackTargets)
      if (TrackIndex{track}.staffIndex() == staff)
      {
        if (onTick)
          due(targets.onTick);
        due(targets.offTick);
      }
  };
  for (auto &[staff, targets] : m_staffTargets)
    if (targets.onTick && now >= *targets.onTick)
    {
      // Advancing releases the previous chord itself: the pending release
      // is superseded.
      targets.onTick.reset();
      targets.offTick.reset();
      forget(staff, true);
      Fire(staff, true);
    }
    else if (targets.offTick && now >= *targets.offTick)
    {
      targets.offTick.reset();
      forget(staff, false);
      Fire(staff, false);
    }
}

bool FollowPlayer::Join(double performedTick)
{
  std::vector<int> late;
  for (const auto &[staff, targets] : m_staffTargets)
    if (targets.onTick && *targets.onTick < performedTick)
      late.push_back(staff);
  // Not while iterating: the skip's transitions rebuild the targets.
  for (const int staff : late)
    m_sequencer.SkipHandTo(staff > 0,
                           static_cast<int>(std::ceil(performedTick)));
  return !late.empty();
}

void FollowPlayer::Fire(int staff, bool noteOn)
{
  // Deferred out of the poll: the gesture's transitions come back here with
  // fresh targets.
  muse::async::Async::call(this,
                           [this, staff, noteOn]
                           {
                             if (!IsFollowStaff(staff))
                               return;
                             m_sequencer.OnHandInputEvent(
                                 noteOn ? NoteEventType::noteOn
                                        : NoteEventType::noteOff,
                                 staff > 0, std::nullopt);
                           });
}
} // namespace dgk
