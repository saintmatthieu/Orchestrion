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
