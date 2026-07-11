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

#include "ILoopBoundariesController.h"
#include "IOrchestrionNotationInteractionProcessor.h"
#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <context/iglobalcontext.h>
#include <modularity/ioc.h>
#include <notation/inotationplayback.h>

namespace dgk
{
class LoopBoundariesController : public ILoopBoundariesController,
                                 public muse::Injectable,
                                 public muse::async::Asyncable,
                                 public muse::actions::Actionable
{
  muse::Inject<mu::context::IGlobalContext> globalContext = {this};
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher = {this};
  muse::Inject<IOrchestrionNotationInteractionProcessor>
      interactionProcessor = {this};

public:
  void init();

  std::optional<ChordTicks>
  chordTicks(const mu::notation::EngravingItem *) const override;
  void onChordShiftClicked(const ChordTicks &) override;
  void setLoopStart(int tick) override;
  void setLoopEnd(int endTick) override;
  void clearLoop() override;

private:
  mu::notation::INotationPlaybackPtr playback() const;

  // Chord of the last plain click, target of the loop-in/loop-out actions.
  std::optional<ChordTicks> m_lastClicked;
  // A shift-click pair is in progress: the start was set, the next
  // shift-click on a later chord sets the end.
  bool m_awaitingLoopEnd = false;
};
} // namespace dgk
