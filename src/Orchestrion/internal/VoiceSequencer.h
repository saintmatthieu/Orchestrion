#pragma once

#include "OrchestrionTypes.h"
#include <async/channel.h>
#include <audio/OrchestrionEventTypes.h>
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
  VoiceSequencer(int track, int voice, std::vector<ChordPtr> chords);

  const int track;
  const int voice;

  Next OnInputEvent(NoteEvent::Type, int midiPitch,
                    const dgk::Tick &cursorTick);
  //! Returns noteoffs that were pending.
  std::vector<int> GoToTick(int tick);

  std::optional<dgk::Tick> GetNextTick(NoteEvent::Type) const;
  dgk::Tick GetFinalTick() const;
  std::optional<dgk::Tick> GetTickForPedal() const;
  muse::async::Channel<ChordActivationChange> ChordActivationChanged() const;

private:
  void Advance(NoteEvent::Type, int midiPitch, const dgk::Tick &cursorTick);
  int GetNextBegin(NoteEvent::Type) const;

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
