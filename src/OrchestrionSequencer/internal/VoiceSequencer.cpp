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
#include "VoiceSequencer.h"
#include "IChord.h"
#include "IMelodySegment.h"
#include "IRest.h"
#include <algorithm>
#include <cassert>

namespace dgk
{
VoiceEvent
VoiceSequencer::GetVoiceEvent(const std::vector<ChordRestPtr> &chords,
                              int index)
{
  if (index < 0 || chords.size() <= index)
    return VoiceEvent::finalRest;
  else if (chords[index]->AsChord())
    return VoiceEvent::chord;
  else
    return VoiceEvent::rest;
}

VoiceSequencer::VoiceSequencer(TrackIndex track,
                               std::vector<ChordRestPtr> chords)
    : track{std::move(track)}, m_gestures{std::move(chords)},
      m_numGestures{static_cast<int>(m_gestures.size())}
{
}

ChordTransitionType
VoiceSequencer::GetNextTransition(NoteEventType event,
                                  const Tick &cursorTick) const
{
  const VoiceEvent prev =
      m_onImplicitRest ? VoiceEvent::none : GetVoiceEvent(m_gestures, m_index);
  const VoiceEvent next =
      GetVoiceEvent(m_gestures, m_onImplicitRest ? m_index : m_index + 1);
  const auto transition = event == NoteEventType::noteOn
                              ? CTU::GetTransitionForNoteon(prev, next)
                              : CTU::GetTransitionForNoteoff(prev, next);
  if (transition == ChordTransitionType::chordToChordOverSkippedRest &&
      GetFutureChord()->GetBeginTick() > cursorTick)
    // A little amendmend ...
    return ChordTransitionType::chordToRest;
  else
    return transition;
}

namespace
{
int GetIndexIncrement(ChordTransitionType transition)
{
  assert(static_cast<int>(ChordTransitionType::_count) == 7);
  // Principle: the index is incremented when a chord or rest is finished.
  switch (transition)
  {
  case ChordTransitionType::none:
  case ChordTransitionType::implicitRestToChord:
    return 0;
  case ChordTransitionType::chordToImplicitRest:
  case ChordTransitionType::chordToChord:
  case ChordTransitionType::chordToRest:
  case ChordTransitionType::restToChord:
    return 1;
  case ChordTransitionType::chordToChordOverSkippedRest:
    return 2;
  }
  assert(false);
  return 0;
}
} // namespace

std::optional<ChordTransition>
VoiceSequencer::OnInputEvent(NoteEventType event, const dgk::Tick &cursorTick)
{
  if (m_index >= m_numGestures ||
      cursorTick < GetNextMatchingTick(event, cursorTick))
    return std::nullopt;

  const auto transition = GetNextTransition(event, cursorTick);
  const auto indexIncrement = GetIndexIncrement(transition);
  m_index += indexIncrement;

  if (transition != ChordTransitionType::none)
    m_onImplicitRest = transition == ChordTransitionType::chordToImplicitRest;

  static_assert(static_cast<int>(ChordTransitionType::_count) == 7);
  switch (transition)
  {
  case ChordTransitionType::none:
    return std::nullopt;
  case ChordTransitionType::chordToChord:
    return PastChordAndPresentChord{m_gestures[m_index - 1]->AsChord(),
                                    m_gestures[m_index]->AsChord()};
  case ChordTransitionType::chordToChordOverSkippedRest:
    return PastChordAndPresentChord{m_gestures[m_index - 2]->AsChord(),
                                    m_gestures[m_index]->AsChord()};
  case ChordTransitionType::chordToImplicitRest:
  {
    const auto past = m_gestures[m_index - 1]->AsChord();
    const auto future = GetFutureChord();
    if (future)
      return PastChordAndFutureChord{past, future};
    else
      return PastChord{past};
  }
  case ChordTransitionType::chordToRest:
    if (m_index < m_numGestures)
      return PastChordAndPresentRest{m_gestures[m_index - 1]->AsChord(),
                                     m_gestures[m_index]->AsRest()};
    else
      return PastChord{m_gestures[m_index - 1]->AsChord()};
  case ChordTransitionType::restToChord:
  case ChordTransitionType::implicitRestToChord:
    return PresentChord{m_gestures[m_index]->AsChord()};
  default:
    assert(false);
    return std::nullopt;
  }
}

const IChord *VoiceSequencer::GetFutureChord() const
{
  auto index = m_index;
  if (!m_onImplicitRest)
    ++index;
  while (index < m_numGestures)
  {
    if (const auto chord = m_gestures[index]->AsChord())
      return chord;
    ++index;
  }
  return nullptr;
}

const IMelodySegment *VoiceSequencer::GetPresentThing() const
{
  return m_onImplicitRest ? nullptr : m_gestures[m_index].get();
}

std::optional<ChordTransition> VoiceSequencer::GoToTick(int tick)
{
  const IChord *const past = m_onImplicitRest || m_index >= m_numGestures
                                 ? nullptr
                                 : m_gestures[m_index]->AsChord();
  m_onImplicitRest = true;

  for (auto i = 0; i < m_gestures.size(); ++i)
    if (m_gestures[i]->AsChord() &&
        m_gestures[i]->GetBeginTick().withoutRepeats >= tick)
    {
      m_index = i;
      break;
    }

  const IChord *const future = GetFutureChord();
  if (past && !future)
    return PastChord{past};
  else if (!past && future)
    return FutureChord{future};
  else if (past && future)
    return PastChordAndFutureChord{past, future};
  else
    return std::nullopt;
}

std::optional<dgk::Tick>
VoiceSequencer::GetNextMatchingTick(NoteEventType event) const
{
  if (event == NoteEventType::noteOff)
    return GetNextMatchingTickForNoteoff();
  else
    // For querying purpose ; ignore rests.
    return GetNextMatchingTickForNoteon(true);
}

std::optional<dgk::Tick> VoiceSequencer::GetNextMatchingTick(
    NoteEventType event, const std::optional<dgk::Tick> &upperLimit) const
{
  if (event == NoteEventType::noteOff)
    return GetNextMatchingTickForNoteoff();

  if (m_index >= m_numGestures)
    return std::nullopt;

  if (!upperLimit.has_value())
  {
    if (const auto future = GetFutureChord())
      return future->GetBeginTick();
    else
    {
      if (m_onImplicitRest)
        return m_gestures[m_index]->GetBeginTick();
      else
        return m_gestures[m_index]->GetEndTick();
    }
  }
  else
  {
    const auto future = GetFutureChord();
    if (future && future->GetBeginTick() <= *upperLimit)
      return future->GetBeginTick();
    else
      return m_gestures[m_index]->GetEndTick();
  }
}

std::optional<dgk::Tick>
VoiceSequencer::GetNextMatchingTickForNoteon(bool skippingRests) const
{
  if (m_index == m_numGestures)
    return std::nullopt;

  if (skippingRests)
  {
    if (const auto future = GetFutureChord())
      return future->GetBeginTick();
    else
      return std::nullopt;
  }
  else
  {
    const auto index = m_onImplicitRest ? m_index : m_index + 1;
    return index <= m_numGestures
               ? std::make_optional(m_gestures[index]->GetBeginTick())
               : std::nullopt;
  }
}

std::optional<dgk::Tick> VoiceSequencer::GetNextMatchingTickForNoteoff() const
{
  if (m_index == m_numGestures)
    return std::nullopt;
  else if (m_onImplicitRest || m_gestures[m_index]->AsRest())
    return std::nullopt;
  else
    return m_gestures[m_index]->AsChord()->GetEndTick();
}

std::optional<dgk::Tick> VoiceSequencer::GetTickForPedal() const
{
  // m_index is also the end of the gestures consumed so far.
  // We add to this the upcoming gesture if this is a rest.
  if (m_index == m_numGestures)
    return std::nullopt;
  else if (m_gestures[m_index]->AsChord() || m_index + 1 == m_numGestures)
    return m_gestures[m_index]->GetBeginTick();
  else
    return std::nullopt;
}

dgk::Tick VoiceSequencer::GetFinalTick() const
{
  return m_gestures.empty() ? dgk::Tick{0, 0} : m_gestures.back()->GetEndTick();
}

} // namespace dgk
