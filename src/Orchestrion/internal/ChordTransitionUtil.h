#pragma once

#include "OrchestrionTypes.h"

namespace dgk
{

enum class ChordTransitionType
{
  none,
  implicitRestToChord,
  implicitRestToChordOverSkippedRest,
  restToChord,
  restToImplicitRest,
  chordToImplicitRest,
  chordToChord,
  chordToRest,
  chordToChordOverSkippedRest,
  _count
};

enum class VoiceEvent
{
  none,
  chord,
  rest,
  finalRest,
  _count
};

namespace CTU
{
ChordTransitionType GetTransitionForNoteon(VoiceEvent prev, VoiceEvent next);
ChordTransitionType GetTransitionForNoteoff(VoiceEvent prev, VoiceEvent next);
} // namespace CTU

} // namespace dgk