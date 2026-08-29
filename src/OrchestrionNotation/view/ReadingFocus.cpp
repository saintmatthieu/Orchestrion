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
#include "ReadingFocus.h"

namespace dgk
{
void ReadingFocus::onTransition(int staff, int track,
                                std::optional<Onset> present,
                                std::optional<Onset> future)
{
  Voice &voice = _voices[track];
  voice.staff = staff;
  if (present)
  {
    // A strike (or a move onto a rest): the pre-light it consumes is gone;
    // the release will bring the next.
    voice.last = present;
    voice.next.reset();
  }
  if (future)
    voice.next = future;
}

void ReadingFocus::clear() { _voices.clear(); }

std::optional<ReadingFocus::Onset> ReadingFocus::leaderOnset() const
{
  // The voices of a staff are played by the same gestures: the hand is where
  // the furthest of them is.
  std::optional<Onset> leader;
  for (const auto &[track, voice] : _voices)
    if (voice.last && (!leader || voice.last->utick > leader->utick))
      leader = voice.last;
  return leader;
}

std::optional<ReadingFocus::Onset> ReadingFocus::focus() const
{
  // Per hand: where it last struck (the furthest of its voices), and its next
  // event — the most imminent pre-lit chord among its voices, not counting
  // one a voice was left holding from before the hand moved on.
  struct Hand
  {
    std::optional<Onset> last;
    std::optional<Onset> next;
  };
  std::map<int /*staff*/, Hand> hands;
  for (const auto &[track, voice] : _voices)
  {
    Hand &hand = hands[voice.staff];
    if (voice.last && (!hand.last || voice.last->utick > hand.last->utick))
      hand.last = voice.last;
  }
  for (const auto &[track, voice] : _voices)
  {
    Hand &hand = hands[voice.staff];
    if (!voice.next || (hand.last && voice.next->utick < hand.last->utick))
      continue;
    if (!hand.next || voice.next->utick < hand.next->utick)
      hand.next = voice.next;
  }

  const std::optional<Onset> leader = leaderOnset();
  std::optional<Onset> focus;
  for (const auto &[staff, hand] : hands)
  {
    // What the hand plays next — or, between a strike and the release that
    // pre-lights the next, what it sounds.
    const std::optional<Onset> &next = hand.next ? hand.next : hand.last;
    // A hand whose next note lies behind what the leader has already struck
    // is trailing: the page does not wait for it.
    if (!next || (leader && next->utick < leader->utick))
      continue;
    if (!focus || next->utick < focus->utick)
      focus = next;
  }
  return focus;
}
} // namespace dgk
