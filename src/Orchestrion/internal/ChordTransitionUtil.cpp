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
      assert(false);
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
      return ChordTransitionType::implicitRestToChord;
    case VoiceEvent::rest:
      return ChordTransitionType::implicitRestToChordOverSkippedRest;
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

ChordTransitionType
CTU::GetTransitionForNoteoff(VoiceEvent prev, VoiceEvent next,
                             const std::optional<uint8_t> &lastKey,
                             uint8_t midiPitch)
{
  if (lastKey != midiPitch)
    return ChordTransitionType::none;

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
    // Shouldn't be possible.
    assert(false);
  case VoiceEvent::none:
    // Must have reached the end and still be playing.
    return ChordTransitionType::none;
  }

  assert(false);
  return ChordTransitionType::none;
}
} // namespace dgk