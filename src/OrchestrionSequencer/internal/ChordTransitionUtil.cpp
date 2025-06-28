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
#include "ChordTransitionUtil.h"
#include <cassert>

namespace dgk
{
ChordTransitionType CTU::GetTransitionForNoteon(VoiceEvent prev,
                                                VoiceEvent next)
{
  switch (prev)
  {
  case VoiceEvent::chord:
    switch (next)
    {
    case VoiceEvent::chord:
      return ChordTransitionType::chordToChord;
    case VoiceEvent::rest:
      return ChordTransitionType::chordToChordOverSkippedRest;
    case VoiceEvent::finalRest:
      return ChordTransitionType::chordToRest;
    case VoiceEvent::none:
      return ChordTransitionType::none;
    }

  case VoiceEvent::rest:
    switch (next)
    {
    case VoiceEvent::chord:
      return ChordTransitionType::restToChord;
    case VoiceEvent::rest:
    case VoiceEvent::finalRest:
      return ChordTransitionType::none;
    case VoiceEvent::none:
      return ChordTransitionType::none;
    }

  case VoiceEvent::finalRest:
    assert(next == VoiceEvent::none);
    return ChordTransitionType::none;

  case VoiceEvent::none:
    switch (next)
    {
    case VoiceEvent::chord:
    case VoiceEvent::rest:
      return ChordTransitionType::implicitRestToChord;
    case VoiceEvent::finalRest:
      assert(false);
      return ChordTransitionType::none;
    case VoiceEvent::none:
      return ChordTransitionType::none;
    }
  }

  assert(false);
  return ChordTransitionType::none;
}

ChordTransitionType CTU::GetTransitionForNoteoff(VoiceEvent prev,
                                                 VoiceEvent next)
{
  switch (prev)
  {
  case VoiceEvent::chord:
    switch (next)
    {
    case VoiceEvent::chord:
      return ChordTransitionType::chordToImplicitRest;
    case VoiceEvent::rest:
    case VoiceEvent::finalRest:
      return ChordTransitionType::chordToRest;
    case VoiceEvent::none:
      return ChordTransitionType::chordToImplicitRest;
    }

  case VoiceEvent::rest:
  case VoiceEvent::finalRest:
  case VoiceEvent::none:
    // Must have reached the end and still be playing.
    return ChordTransitionType::none;
  }

  assert(false);
  return ChordTransitionType::none;
}
} // namespace dgk