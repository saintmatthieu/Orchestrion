#include "VoiceSequencer.h"
#include "IChord.h"
#include <algorithm>
#include <cassert>

namespace dgk
{
VoiceSequencer::VoiceSequencer(TrackIndex track, std::vector<ChordPtr> chords)
    : track{std::move(track)}, m_gestures{std::move(chords)},
      m_numGestures{static_cast<int>(m_gestures.size())}
{
}

dgk::VoiceSequencer::Next
VoiceSequencer::OnInputEvent(NoteEventType event, int midiPitch,
                             const dgk::Tick &cursorTick)
{
  const auto before = m_active;
  Advance(event, midiPitch, cursorTick);

  std::vector<int> noteOffs;
  std::vector<const IChord *> deactivated;
  for (auto i = before.begin; i < m_active.begin; ++i)
  {
    const auto &gesture = m_gestures[i];
    const auto pitches = gesture->GetPitches();
    deactivated.push_back(gesture.get());
    noteOffs.insert(noteOffs.end(), pitches.begin(), pitches.end());
  }

  std::vector<int> noteOns;
  for (auto i = before.end; i < m_active.end; ++i)
  {
    const auto pitches = m_gestures[i]->GetPitches();
    noteOns.insert(noteOns.end(), pitches.begin(), pitches.end());
  }

  // Remove entries in noteOffs that are in noteOns
  noteOffs.erase(std::remove_if(noteOffs.begin(), noteOffs.end(),
                                [&noteOns](int note)
                                {
                                  return std::find(noteOns.begin(),
                                                   noteOns.end(),
                                                   note) != noteOns.end();
                                }),
                 noteOffs.end());

  const IChord *activated = m_active.begin < m_active.end
                                ? m_gestures[m_active.end - 1].get()
                                : nullptr;

  m_chordActivationChanged.send({deactivated, activated});

  return {noteOns, noteOffs};
}

void VoiceSequencer::Advance(NoteEventType event, int midiPitch,
                             const dgk::Tick &cursorTick)
{
  Finally finally{[&]
                  {
                    if (event == NoteEventType::noteOn)
                      m_pressedKey = midiPitch;
                    else if (m_pressedKey == midiPitch)
                      m_pressedKey.reset();
                  }};

  if (event == NoteEventType::noteOff && m_pressedKey.has_value() &&
      *m_pressedKey != midiPitch)
    // Probably playing with two or more fingers, legato style - ignore
    return;

  // If there is something active, then the end tick of the first active
  // gesture. Else, the tick of the next gesture.
  const auto nextBegin = GetNextBegin(event);

  if (nextBegin == m_numGestures)
  {
    // `nextBegin` might show the end for this event type, yet don't switch
    // anything off until the cursor has reached it.
    if (m_active.end == m_numGestures ||
        cursorTick >= m_gestures[m_active.end]->GetBeginTick())
      // Ok then.
      m_active.begin = m_active.end = nextBegin;
    return;
  }

  if (cursorTick < m_gestures[nextBegin]->GetBeginTick())
    // We're finished or the cursor hasn't reached our next event yet.
    return;

  m_active.begin = nextBegin;
  // Do not open up to the next chord if the user is releasing the key.
  m_active.end =
      m_gestures[nextBegin]->IsChord() && event == NoteEventType::noteOff
          ? nextBegin
          : nextBegin + 1;
}

std::vector<int> VoiceSequencer::GoToTick(int tick)
{

  std::vector<int> noteOffs;
  for (auto i = m_active.begin; i < m_active.end; ++i)
  {
    const auto &gesture = m_gestures[i];
    const auto pitches = gesture->GetPitches();
    noteOffs.insert(noteOffs.end(), pitches.begin(), pitches.end());
  }

  m_active.begin = m_active.end = m_numGestures;
  for (auto i = 0; i < m_gestures.size(); ++i)
    if (m_gestures[i]->IsChord() &&
        m_gestures[i]->GetBeginTick().withoutRepeats >= tick)
    {
      m_active.begin = m_active.end = i;
      break;
    }

  m_pressedKey.reset();
  return noteOffs;
}

int VoiceSequencer::GetNextBegin(NoteEventType event) const
{
  auto nextActive = m_active.end;

  if (nextActive == m_numGestures)
    // The end.
    return m_numGestures;

  const auto isChord = m_gestures[nextActive]->IsChord();
  // Skip the next rest if the user is pressing the key.
  return !isChord && event == NoteEventType::noteOn ? nextActive + 1
                                                    : nextActive;
}

std::optional<dgk::Tick> VoiceSequencer::GetNextTick(NoteEventType event) const
{
  const auto i = GetNextBegin(event);
  return i < m_numGestures ? std::make_optional(m_gestures[i]->GetBeginTick())
                           : std::nullopt;
}

std::optional<dgk::Tick> VoiceSequencer::GetTickForPedal() const
{
  // m_active.begin is also the end of the gestures consumed so far.
  // We add to this the upcoming gesture if this is a rest.
  if (m_active.begin == m_numGestures)
    return std::nullopt;
  else if (m_gestures[m_active.begin]->IsChord() ||
           m_active.begin + 1 == m_numGestures)
    return m_gestures[m_active.begin]->GetBeginTick();
  else
    return std::nullopt;
}

muse::async::Channel<ChordActivationChange>
VoiceSequencer::ChordActivationChanged() const
{
  return m_chordActivationChanged;
}

dgk::Tick VoiceSequencer::GetFinalTick() const
{
  return m_gestures.empty() ? dgk::Tick{0, 0} : m_gestures.back()->GetEndTick();
}

} // namespace dgk
