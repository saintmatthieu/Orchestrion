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

#include "IScoreAnimator.h"
#include "ISegmentRegistry.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/OrchestrionTypes.h"
#include <async/asyncable.h>
#include <context/iglobalcontext.h>
#include <modularity/ioc.h>
#include <notation/inotationinteraction.h>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace mu::engraving
{
class Segment;
}

namespace dgk
{
class ScoreAnimator : public IScoreAnimator,
                      public dgk::Injectable,
                      public muse::async::Asyncable
{
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<ISegmentRegistry> melodySegRegistry{this};
  dgk::Inject<mu::context::IGlobalContext> globalContext{this};

public:
  void init();

private:
  void Subscribe(const IOrchestrionSequencer &sequencer);
  void OnChordTransitions(const std::map<TrackIndex, ChordTransition> &);
  mu::notation::INotationInteractionPtr GetInteraction() const;
};
} // namespace dgk
