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
#include "GestureInputConnector.h"

#include <log.h>

namespace dgk
{
void GestureInputConnector::init()
{
  gestureInput()->noteOn().onReceive(
      this, [this](int pitch, std::optional<float> velocity)
      { onNoteOn(pitch, std::move(velocity)); });
  gestureInput()->noteOff().onReceive(this,
                                      [this](int pitch) { onNoteOff(pitch); });
}

void GestureInputConnector::onNoteOn(int pitch, std::optional<float> velocity)
{
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    // Things don't look ready yet
    return;
  sequencer->OnInputEvent(NoteEventType::noteOn, pitch, std::move(velocity));
}

void GestureInputConnector::onNoteOff(int pitch)
{
  const auto sequencer = orchestrion()->sequencer();
  IF_ASSERT_FAILED(sequencer) return;
  sequencer->OnInputEvent(NoteEventType::noteOff, pitch, 0.0f);
}
} // namespace dgk
