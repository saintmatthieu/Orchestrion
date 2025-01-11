#include "VoiceSequencer.h"
#include "IChord.h"
#include <algorithm>
#include <cassert>

namespace dgk
{
VoiceEvent VoiceSequencer::GetVoiceEvent(const std::vector<ChordPtr> &chords,
                                         int index)
{
  if (index >= chords.size())
    return VoiceEvent::none;
  else if (chords[index]->IsChord())
    return VoiceEvent::chord;
  else if (index + 1 == chords.size())
    return VoiceEvent::finalRest;
  else
    return VoiceEvent::rest;
}

VoiceSequencer::VoiceSequencer(TrackIndex track, std::vector<ChordPtr> chords)
    : track{std::move(track)}, m_gestures{std::move(chords)},
      m_numGestures{static_cast<int>(m_gestures.size())}
{
}

ChordTransitionType VoiceSequencer::GetNextTransition(NoteEventType event,
                                                      uint8_t midiPitch) const
{
  const VoiceEvent prev =
      m_onImplicitRest ? VoiceEvent::none : GetVoiceEvent(m_gestures, m_index);
  const VoiceEvent next =
      GetVoiceEvent(m_gestures, m_onImplicitRest ? m_index : m_index + 1);
  return event == NoteEventType::noteOn
             ? CTU::GetTransitionForNoteon(prev, next)
             : CTU::GetTransitionForNoteoff(prev, next, m_pressedKey,
                                            midiPitch);
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

const IChord *VoiceSequencer::GetFirstChord() const
{
  const auto it =
      std::find_if(m_gestures.begin(), m_gestures.end(),
                   [](const auto &gesture) { return gesture->IsChord(); });
  return it != m_gestures.end() ? it->get() : nullptr;
}

ChordTransition VoiceSequencer::OnInputEvent(NoteEventType event, int midiPitch,
                                             const dgk::Tick &cursorTick)
{
  const ChordTransitionType transitionType =
      GetNextTransition(event, midiPitch);

  if (event == NoteEventType::noteOn)
    m_pressedKey = midiPitch;
  else if (m_pressedKey == midiPitch)
    m_pressedKey.reset();

  m_index += GetIndexIncrement(transitionType);
  if (transitionType != ChordTransitionType::none)
    m_onImplicitRest =
        transitionType == ChordTransitionType::chordToImplicitRest ||
        transitionType == ChordTransitionType::restToImplicitRest;

  switch (transitionType)
  {
  case ChordTransitionType::none:
    return {};
  case ChordTransitionType::chordToChordOverSkippedRest:
    return {ChordTransition::Deactivated{m_gestures[m_index - 2].get()},
            ChordTransition::SkippedRest{m_gestures[m_index - 1].get()},
            ChordTransition::Activated{m_gestures[m_index].get()}};
  case ChordTransitionType::chordToImplicitRest:
  case ChordTransitionType::restToImplicitRest:
    return {ChordTransition::Deactivated{m_gestures[m_index - 1].get()},
            ChordTransition::Next{GetChord(m_index)}};
  case ChordTransitionType::chordToChord:
  case ChordTransitionType::chordToRest:
  case ChordTransitionType::restToChord:
    return {ChordTransition::Deactivated{m_gestures[m_index - 1].get()},
            ChordTransition::Activated{m_gestures[m_index].get()}};
  case ChordTransitionType::implicitRestToChord:
    return {ChordTransition::Activated{m_gestures[m_index].get()}};
  case ChordTransitionType::implicitRestToChordOverSkippedRest:
    return {ChordTransition::SkippedRest{m_gestures[m_index - 1].get()},
            ChordTransition::Activated{m_gestures[m_index].get()}};
  default:
    assert(false);
    return {};
  }
}

const IChord *VoiceSequencer::GetChord(int index) const
{
  return index < m_numGestures ? m_gestures[index].get() : nullptr;
}

ChordTransition VoiceSequencer::GoToTick(int tick)
{
  const auto prevIndex = m_index;

  for (auto i = 0; i < m_gestures.size(); ++i)
    if (m_gestures[i]->IsChord() &&
        m_gestures[i]->GetBeginTick().withoutRepeats >= tick)
    {
      m_index = i;
      break;
    }

  Finally finally{[this]
                  {
                    m_onImplicitRest = true;
                    m_pressedKey.reset();
                  }};

  if (m_onImplicitRest)
    return {ChordTransition::Next{GetChord(m_index)}};
  else
    return {ChordTransition::Deactivated{GetChord(prevIndex)},
            ChordTransition::Next{GetChord(m_index)}};
}

int VoiceSequencer::GetNextIndex(NoteEventType event) const
{
  const VoiceEvent prev = GetVoiceEvent(m_gestures, m_index);
  const VoiceEvent next = GetVoiceEvent(m_gestures, m_index + 1);
  const auto transition = CTU::GetTransitionForNoteon(prev, next);
  return m_index + GetIndexIncrement(transition);
}

std::optional<dgk::Tick> VoiceSequencer::GetNextTick(NoteEventType event) const
{
  const auto i = GetNextIndex(event);
  return i < m_numGestures ? std::make_optional(m_gestures[i]->GetBeginTick())
                           : std::nullopt;
}

std::optional<dgk::Tick> VoiceSequencer::GetTickForPedal() const
{
  // m_index is also the end of the gestures consumed so far.
  // We add to this the upcoming gesture if this is a rest.
  if (m_index == m_numGestures)
    return std::nullopt;
  else if (m_gestures[m_index]->IsChord() || m_index + 1 == m_numGestures)
    return m_gestures[m_index]->GetBeginTick();
  else
    return std::nullopt;
}

dgk::Tick VoiceSequencer::GetFinalTick() const
{
  return m_gestures.empty() ? dgk::Tick{0, 0} : m_gestures.back()->GetEndTick();
}

} // namespace dgk
