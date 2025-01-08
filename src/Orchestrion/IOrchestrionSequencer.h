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

#include "OrchestrionTypes.h"
#include <async/channel.h>

namespace dgk
{
class IOrchestrionSequencer
{
public:
  virtual ~IOrchestrionSequencer() = default;

  virtual void OnInputEvent(NoteEventType, int pitch, float velocity) = 0;
  virtual void GoToTick(int tick) = 0;
  virtual InstrumentIndex GetInstrumentIndex() const = 0;
  virtual muse::async::Channel<TrackIndex, ChordTransition>
  ChordTransitionTriggered() const = 0;
  virtual muse::async::Channel<EventVariant> OutputEvent() const = 0;
};
} // namespace dgk