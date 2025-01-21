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
  VoiceSequencer(TrackIndex, std::vector<ChordRestPtr> chords);

  const TrackIndex track;

  std::optional<ChordTransition> OnInputEvent(NoteEventType,
                                              const dgk::Tick &cursorTick);
  std::optional<ChordTransition> GoToTick(int tick);

  std::optional<dgk::Tick> GetNextNoteonTick() const;
  dgk::Tick GetFinalTick() const;
  std::optional<dgk::Tick> GetTickForPedal() const;
  const IChord *GetNextChord() const;

private:
  static VoiceEvent GetVoiceEvent(const std::vector<ChordRestPtr> &chords,
                                  int index);
  ChordTransitionType GetNextTransition(NoteEventType event) const;
  const IChord *GetNextChord(int index) const;

  const std::vector<ChordRestPtr> m_gestures;
  const int m_numGestures;
  int m_index = 0;
  bool m_onImplicitRest = true;
};
} // namespace dgk
