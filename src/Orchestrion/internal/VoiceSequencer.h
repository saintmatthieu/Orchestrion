#pragma once

#include "ChordTransitionUtil.h"
#include "OrchestrionTypes.h"
#include <optional>
#include <vector>

namespace dgk
{
class VoiceSequencer
{
public:
  VoiceSequencer(TrackIndex, std::vector<ChordPtr> chords);

  const TrackIndex track;

  ChordTransition OnInputEvent(NoteEventType, const dgk::Tick &cursorTick);
  //! Returns noteoffs that were pending.
  ChordTransition GoToTick(int tick);

  std::optional<dgk::Tick> GetNextTick(NoteEventType) const;
  dgk::Tick GetFinalTick() const;
  std::optional<dgk::Tick> GetTickForPedal() const;
  const IChord *GetFirstChord() const;

private:
  static VoiceEvent GetVoiceEvent(const std::vector<ChordPtr> &chords,
                                  int index);
  ChordTransitionType GetNextTransition(NoteEventType event) const;
  const IChord *GetChord(int index) const;

  const std::vector<ChordPtr> m_gestures;
  const int m_numGestures;
  int m_index = 0;
  bool m_onImplicitRest = true;
};
} // namespace dgk
