#pragma once

#include "OrchestrionTypes.h"
#include <async/channel.h>
#include <optional>
#include <vector>

namespace dgk
{
class VoiceSequencer
{
public:
  struct Next
  {
    std::vector<int> noteOns;
    std::vector<int> noteOffs;
  };

public:
  VoiceSequencer(TrackIndex, std::vector<ChordPtr> chords);

  const TrackIndex track;

  Next OnInputEvent(NoteEventType, int midiPitch,
                    const dgk::Tick &cursorTick);
  //! Returns noteoffs that were pending.
  std::vector<int> GoToTick(int tick);

  std::optional<dgk::Tick> GetNextTick(NoteEventType) const;
  dgk::Tick GetFinalTick() const;
  std::optional<dgk::Tick> GetTickForPedal() const;
  muse::async::Channel<ChordActivationChange> ChordActivationChanged() const;

private:
  void Advance(NoteEventType, int midiPitch, const dgk::Tick &cursorTick);
  int GetNextBegin(NoteEventType) const;

  struct Range
  {
    int begin = 0;
    int end = 0;
  };

  const std::vector<ChordPtr> m_gestures;
  const int m_numGestures;
  Range m_active; // Range of active chords / rests.
  std::optional<uint8_t> m_pressedKey;
  muse::async::Channel<ChordActivationChange> m_chordActivationChanged;
};
} // namespace dgk
