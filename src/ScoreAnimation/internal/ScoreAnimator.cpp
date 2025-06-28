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
#include "ScoreAnimator.h"
#include "OrchestrionSequencer/IChord.h"
#include "OrchestrionSequencer/IOrchestrionSequencer.h"
#include "OrchestrionSequencer/OrchestrionTypes.h"
#include <engraving/dom/chord.h>
#include <engraving/dom/masterscore.h>
#include <engraving/dom/note.h>
#include <engraving/dom/tie.h>
#include <log.h>

namespace dgk
{
void ScoreAnimator::init()
{
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this]
                                             {
                                               const auto sequencer =
                                                   orchestrion()->sequencer();
                                               if (!sequencer)
                                                 return;
                                               Subscribe(*sequencer);
                                             });
  if (const auto sequencer = orchestrion()->sequencer())
    Subscribe(*sequencer);
}

void ScoreAnimator::Subscribe(const IOrchestrionSequencer &sequencer)
{
  sequencer.ChordTransitions().onReceive(
      this, [this](std::map<TrackIndex, ChordTransition> transitions)
      { OnChordTransitions(transitions); });
}

void ScoreAnimator::OnChordTransitions(
    const std::map<TrackIndex, ChordTransition> &transitions)
{
  const auto interaction = GetInteraction();
  IF_ASSERT_FAILED(interaction) { return; }
  for (const auto &[track, transition] : transitions)
  {
    const auto chord = GetPresentChord(transition) ? GetPresentChord(transition)
                       : GetFutureChord(transition) ? GetFutureChord(transition)
                                                    : nullptr;
    if (chord)
    {
      const auto segment = melodySegRegistry()->GetSegment(chord);
      IF_ASSERT_FAILED(segment) { return; }
      interaction->showItem(segment->element(track.value));
    }
  }
  interaction->selectionChanged().notify();
}

mu::notation::INotationInteractionPtr ScoreAnimator::GetInteraction() const
{
  const auto masterNotation = globalContext()->currentMasterNotation();
  if (!masterNotation)
    return nullptr;
  return masterNotation->notation()->interaction();
}
} // namespace dgk