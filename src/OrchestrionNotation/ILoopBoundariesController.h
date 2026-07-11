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

#include <modularity/imoduleinterface.h>
#include <notation/notationtypes.h>
#include <optional>

namespace dgk
{
//! Sets the playback loop boundaries honored by the sequencer, from
//! score-view gestures (shift-click, context menu) or the loop-in/loop-out
//! actions. All ticks are raw score ticks (without repeats), matching
//! EngravingItem::tick().
class ILoopBoundariesController : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(ILoopBoundariesController);

public:
  virtual ~ILoopBoundariesController() = default;

  struct ChordTicks
  {
    int start = 0; //!< begin tick of the chord/rest the item belongs to
    int end = 0;   //!< its end tick — the loop-out point that includes it
  };

  virtual std::optional<ChordTicks>
  chordTicks(const mu::notation::EngravingItem *) const = 0;

  //! Shift-click gesture: the first click sets the loop start; a
  //! later-in-the-score click completes the pair by setting the loop end.
  virtual void onChordShiftClicked(const ChordTicks &) = 0;

  virtual void setLoopStart(int tick) = 0;
  virtual void setLoopEnd(int endTick) = 0;
  virtual void clearLoop() = 0;
};
} // namespace dgk
