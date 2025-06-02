#include "OrchestrionTypes.h"
#include "IChord.h"
#include "IRest.h"

const dgk::IChord *dgk::GetPastChord(const ChordTransition &transition)
{
  if (const auto *t = std::get_if<PastChord>(&transition))
    return t->chord;
  else if (const auto *u = std::get_if<PastChordAndPresentChord>(&transition))
    return u->pastChord;
  else if (const auto *v = std::get_if<PastChordAndFutureChord>(&transition))
    return v->pastChord;
  else if (const auto *w = std::get_if<PastChordAndPresentRest>(&transition))
    return w->pastChord;
  else
    return nullptr;
}

const dgk::IChord *dgk::GetPresentChord(const ChordTransition &transition)
{
  if (const auto *t = std::get_if<PresentChord>(&transition))
    return t->chord;
  else if (const auto *u = std::get_if<PastChordAndPresentChord>(&transition))
    return u->presentChord;
  else
    return nullptr;
}

const dgk::IMelodySegment *
dgk::GetPresentThing(const ChordTransition &transition)
{
  if (const auto *t = std::get_if<PresentChord>(&transition))
    return t->chord;
  else if (const auto *u = std::get_if<PastChordAndPresentChord>(&transition))
    return u->presentChord;
  else if (const auto *v = std::get_if<PastChordAndPresentRest>(&transition))
    return v->presentRest;
  else
    return nullptr;
}

const dgk::IChord *dgk::GetFutureChord(const ChordTransition &transition)
{
  if (const auto *t = std::get_if<FutureChord>(&transition))
    return t->chord;
  else if (const auto *u = std::get_if<PastChordAndFutureChord>(&transition))
    return u->futureChord;
  else
    return nullptr;
}
