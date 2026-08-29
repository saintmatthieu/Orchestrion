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

#include <map>
#include <optional>

namespace dgk
{
//! Where each hand has got to in the score, and from that the event the page
//! scroll keeps in view (see ScoreFollower).
//!
//! Each hand advances through the score on its own gestures, so the two can
//! drift apart: one plays on while the other rests on a note it has not
//! released, or simply stops. The page cannot follow both then, and it
//! follows the *leading* hand — the one whose last onset is furthest into the
//! score. The event it keeps in view is the most imminent upcoming one, in
//! either hand, that is not behind the leader: a hand whose next note lies
//! before what the leader has already struck is trailing, and gets no say
//! until it has caught up. Hands in step never trigger that
//! rule: a hand holding a long note has its next event *ahead* of the other's
//! onsets, not behind them.
//!
//! Transition batches only carry the voices that changed, so this is a
//! ledger: per voice, the last struck onset and the upcoming one, persisted
//! between batches. Positions are unrolled (playback) ticks, which are
//! monotonic through repeats, voltas and jumps; the engraved x rides along
//! for the follower.
class ReadingFocus
{
public:
  struct Onset
  {
    int utick = 0;
    double x = 0.0;
  };

  //! One voice's transition: the chord (or rest) it just moved onto, if any,
  //! and the chord pre-lit as its next, if any. A strike carries only the
  //! former, a release only the latter (or neither, when the voice is
  //! through); the ledger fills in what a batch leaves unsaid.
  void onTransition(int staff, int track, std::optional<Onset> present,
                    std::optional<Onset> future);

  //! Forget all hands (the position jumped, or the score changed).
  void clear();

  //! The leading hand's last onset — where the performer is — or nothing
  //! before any hand has struck.
  std::optional<Onset> leaderOnset() const;

  //! The next event to keep in view (see the class comment) — a hand between
  //! a strike and the release that pre-lights its next standing for what it
  //! sounds; the leader's last onset once nothing is left to play; nothing
  //! before any event.
  std::optional<Onset> focus() const;

private:
  struct Voice
  {
    int staff = 0;
    std::optional<Onset> last; // last struck; survives the release
    //! The chord pre-lit as next. Unset from a strike to its release, and for
    //! good once the voice is through.
    std::optional<Onset> next;
  };
  std::map<int /*track*/, Voice> _voices;
};
} // namespace dgk
