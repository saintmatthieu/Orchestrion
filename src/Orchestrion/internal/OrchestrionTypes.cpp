#include "OrchestrionTypes.h"
#include "IChord.h"
#include "IRest.h"

const dgk::IChord *dgk::GetPastChord(const ChordTransition &transition)
{
  if (const auto *t = std::get_if<PastChord>(&transition))
    return t->past;
  else if (const auto *t = std::get_if<PastChordAndPresentChord>(&transition))
    return t->past;
  else if (const auto *t = std::get_if<PastChordAndFutureChord>(&transition))
    return t->past;
  else if (const auto *t = std::get_if<PastChordAndPresentRest>(&transition))
    return t->past;
  else
    return nullptr;
}

const dgk::IChord *dgk::GetPresentChord(const ChordTransition &transition)
{
  if (const auto *t = std::get_if<PresentChord>(&transition))
    return t->present;
  else if (const auto *t = std::get_if<PastChordAndPresentChord>(&transition))
    return t->present;
  else
    return nullptr;
}

const dgk::IMelodySegment *
dgk::GetPresentThing(const ChordTransition &transition)
{
  if (const auto *t = std::get_if<PresentChord>(&transition))
    return t->present;
  else if (const auto *t = std::get_if<PastChordAndPresentChord>(&transition))
    return t->present;
  else if (const auto *t = std::get_if<PastChordAndPresentRest>(&transition))
    return t->present;
  else
    return nullptr;
}

const dgk::IChord *dgk::GetFutureChord(const ChordTransition &transition)
{
  if (const auto *t = std::get_if<FutureChord>(&transition))
    return t->future;
  else if (const auto *t = std::get_if<PastChordAndFutureChord>(&transition))
    return t->future;
  else
    return nullptr;
}
