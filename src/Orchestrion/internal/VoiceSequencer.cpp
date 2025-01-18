#include "VoiceSequencer.h"
#include "IMelodySegment.h"
#include <algorithm>
#include <cassert>

namespace dgk
{
VoiceEvent
VoiceSequencer::GetVoiceEvent(const std::vector<ChordRestPtr> &chords,
                              int index)
{
  if (index < 0 || chords.size() <= index)
    return VoiceEvent::none;
  else if (chords[index]->AsChord())
    return VoiceEvent::chord;
  else if (index + 1 == chords.size())
    return VoiceEvent::finalRest;
  else
    return VoiceEvent::rest;
}

VoiceSequencer::VoiceSequencer(TrackIndex track,
                               std::vector<ChordRestPtr> chords)
    : track{std::move(track)}, m_gestures{std::move(chords)},
      m_numGestures{static_cast<int>(m_gestures.size())}
{
}

ChordTransitionType VoiceSequencer::GetNextTransition(NoteEventType event) const
{
  const VoiceEvent prev =
      m_onImplicitRest ? VoiceEvent::none : GetVoiceEvent(m_gestures, m_index);
  const VoiceEvent next =
      GetVoiceEvent(m_gestures, m_onImplicitRest ? m_index : m_index + 1);
  return event == NoteEventType::noteOn
             ? CTU::GetTransitionForNoteon(prev, next)
             : CTU::GetTransitionForNoteoff(prev, next);
}

namespace
{
int GetIndexIncrement(ChordTransitionType transition)
{
  assert(static_cast<int>(ChordTransitionType::_count) == 9);
  // Principle: the index is incremented when a chord or rest is finished.
  switch (transition)
  {
  case ChordTransitionType::none:
  case ChordTransitionType::implicitRestToChord:
    return 0;
  case ChordTransitionType::chordToImplicitRest:
  case ChordTransitionType::restToImplicitRest:
  case ChordTransitionType::chordToChord:
  case ChordTransitionType::chordToRest:
  case ChordTransitionType::restToChord:
  case ChordTransitionType::implicitRestToChordOverSkippedRest:
    return 1;
  case ChordTransitionType::chordToChordOverSkippedRest:
    return 2;
  }
}
} // namespace

const IChord *VoiceSequencer::GetNextChord() const
{
  return GetNextChord(m_index);
}

ChordTransition VoiceSequencer::OnInputEvent(NoteEventType event,
                                             const dgk::Tick &cursorTick)
{
  const auto transition = GetNextTransition(event);
  const auto indexIncrement = GetIndexIncrement(transition);

  const auto nextIndex = m_index + indexIncrement;
  if (nextIndex >= m_numGestures ||
      m_gestures[nextIndex]->GetBeginTick() > cursorTick)
    return {};

  m_index = nextIndex;
  if (transition != ChordTransitionType::none)
    m_onImplicitRest = transition == ChordTransitionType::chordToImplicitRest ||
                       transition == ChordTransitionType::restToImplicitRest;

  switch (transition)
  {
  case ChordTransitionType::none:
    return {};
  case ChordTransitionType::chordToChordOverSkippedRest:
    return {ChordTransition::DeactivatedChord{m_gestures[m_index - 2]->AsChord()},
            ChordTransition::SkippedRest{m_gestures[m_index - 1]->AsRest()},
            ChordTransition::ActivatedChordRest{m_gestures[m_index].get()}};
  case ChordTransitionType::chordToImplicitRest:
  case ChordTransitionType::restToImplicitRest:
    return {ChordTransition::DeactivatedChord{m_gestures[m_index - 1]->AsChord()},
            ChordTransition::NextChord{GetNextChord(m_index)}};
  case ChordTransitionType::chordToChord:
  case ChordTransitionType::chordToRest:
  case ChordTransitionType::restToChord:
    return {ChordTransition::DeactivatedChord{m_gestures[m_index - 1]->AsChord()},
            ChordTransition::ActivatedChordRest{m_gestures[m_index].get()}};
  case ChordTransitionType::implicitRestToChord:
    return {ChordTransition::ActivatedChordRest{m_gestures[m_index].get()}};
  case ChordTransitionType::implicitRestToChordOverSkippedRest:
    return {ChordTransition::SkippedRest{m_gestures[m_index - 1]->AsRest()},
            ChordTransition::ActivatedChordRest{m_gestures[m_index].get()}};
  default:
    assert(false);
    return {};
  }
}

const IChord *VoiceSequencer::GetNextChord(int index) const
{
  while (index < m_numGestures)
  {
    if (const auto chord = m_gestures[index]->AsChord())
      return chord;
    ++index;
  }
  return nullptr;
}

ChordTransition VoiceSequencer::GoToTick(int tick)
{
  const auto prevIndex = m_index;

  for (auto i = 0; i < m_gestures.size(); ++i)
    if (m_gestures[i]->AsChord() &&
        m_gestures[i]->GetBeginTick().withoutRepeats >= tick)
    {
      m_index = i;
      break;
    }

  Finally finally{[this] { m_onImplicitRest = true; }};

  if (m_onImplicitRest)
    return {ChordTransition::NextChord{GetNextChord(m_index)}};
  else
    return {ChordTransition::DeactivatedChord{GetNextChord(prevIndex)},
            ChordTransition::NextChord{GetNextChord(m_index)}};
}

std::optional<dgk::Tick> VoiceSequencer::GetNextTick(NoteEventType event) const
{
  const ChordTransitionType transition = GetNextTransition(event);
  const auto i = m_index + GetIndexIncrement(transition);
  return i < m_numGestures ? std::make_optional(m_gestures[i]->GetBeginTick())
                           : std::nullopt;
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
