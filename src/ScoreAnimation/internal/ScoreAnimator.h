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

#include "IChordRegistry.h"
#include "IScoreAnimator.h"
#include "Orchestrion/IOrchestrion.h"
#include <async/asyncable.h>
#include <context/iglobalcontext.h>
#include <modularity/ioc.h>
#include <notation/inotationinteraction.h>

namespace mu::engraving
{
class Segment;
}

namespace dgk
{
struct ChordActivationChange;

namespace orchestrion
{
class ScoreAnimator : public IScoreAnimator,
                      public muse::Injectable,
                      public muse::async::Asyncable
{
  muse::Inject<IOrchestrion> orchestrion;
  muse::Inject<IChordRegistry> chordRegistry;
  muse::Inject<mu::context::IGlobalContext> globalContext;

public:
  void init();

private:
  void Subscribe(const IOrchestrionSequencer &sequencer);
  void OnChordActivationChange(int track, const ChordActivationChange &change);
  std::vector<mu::engraving::EngravingItem *>
  GetRelevantItems(int track, const mu::engraving::Segment *segment) const;
  mu::notation::INotationInteractionPtr GetInteraction() const;
  mu::engraving::MasterScore *GetScore() const;
};
} // namespace orchestrion
} // namespace dgk
