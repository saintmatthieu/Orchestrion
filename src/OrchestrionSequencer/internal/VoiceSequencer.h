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

  dgk::Tick GetFinalTick() const;
  std::optional<dgk::Tick> GetTickForPedal() const;
  std::optional<dgk::Tick> GetNextMatchingTick(NoteEventType) const;

  std::optional<dgk::Tick>
  GetNextMatchingTick(NoteEventType,
                      const std::optional<dgk::Tick> &upperLimit) const;

private:
  static VoiceEvent GetVoiceEvent(const std::vector<ChordRestPtr> &chords,
                                  int index);
  ChordTransitionType GetNextTransition(NoteEventType event,
                                        const Tick &cursorTick) const;
  const IChord *GetFutureChord() const;
  const IMelodySegment *GetPresentThing() const;
  std::optional<dgk::Tick> GetNextMatchingTickForNoteoff() const;
  std::optional<dgk::Tick>
  GetNextMatchingTickForNoteon(bool skippingRests) const;

  const std::vector<ChordRestPtr> m_gestures;
  const int m_numGestures;
  int m_index = 0;
  bool m_onImplicitRest = true;
};
} // namespace dgk
