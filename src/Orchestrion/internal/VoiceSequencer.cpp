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
}
} // namespace

const IChord *VoiceSequencer::GetNextChord() const
{
  return GetNextChord(m_index + 1);
}

std::optional<ChordTransition>
VoiceSequencer::OnInputEvent(NoteEventType event, const dgk::Tick &cursorTick)
{
  const auto transition = GetNextTransition(event);
  const auto indexIncrement = GetIndexIncrement(transition);

  const auto nextIndex = m_index + indexIncrement;
  if (nextIndex >= m_numGestures ||
      m_gestures[nextIndex]->GetBeginTick() > cursorTick)
    return {};

  m_index = nextIndex;
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
    const auto future = GetNextChord(m_index);
    if (future)
      return PastChordAndFutureChord{past, future};
    else
      return PastChord{past};
  }
  case ChordTransitionType::chordToRest:
    return PastChordAndPresentRest{m_gestures[m_index - 1]->AsChord(),
                                   m_gestures[m_index]->AsRest()};
  case ChordTransitionType::restToChord:
  case ChordTransitionType::implicitRestToChord:
    return PresentChord{m_gestures[m_index]->AsChord()};
  default:
    assert(false);
    return std::nullopt;
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

std::optional<ChordTransition> VoiceSequencer::GoToTick(int tick)
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
  {
    if (const auto chord = GetNextChord(m_index))
      return FutureChord{chord};
    else
      return std::nullopt;
  }
  else
  {
    const IChord *const prevChord =
        prevIndex < m_numGestures ? m_gestures[prevIndex]->AsChord() : nullptr;
    const IChord *const nextChord = GetNextChord(m_index);
    if (prevChord && !nextChord)
      return PastChord{prevChord};
    else if (!prevChord && nextChord)
      return FutureChord{nextChord};
    else if (prevChord && nextChord)
      return PastChordAndFutureChord{prevChord, nextChord};
    else
      return std::nullopt;
  }
}

std::optional<dgk::Tick> VoiceSequencer::GetNextNoteonTick() const
{
  const auto nextChord = GetNextChord();
  return nextChord ? std::make_optional<Tick>(nextChord->GetBeginTick())
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
