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
