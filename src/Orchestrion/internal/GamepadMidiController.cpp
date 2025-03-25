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
#include "GamepadMidiController.h"
#include "IOrchestrionSequencer.h"

namespace dgk
{
void GamepadMidiController::init()
{
  gamepad()->buttonPressed().onReceive(this, [this](int button, double velocity)
                                     { buttonPressed(button, velocity); });
  gamepad()->buttonReleased().onReceive(this, [this](int button, double velocity)
                                      { buttonReleased(button, velocity); });

  m_noteMap = {
    {7, 0},
    {9, 0},
    {11, 0},
    {12, 0},
    {13, 0},
    {14, 0},
    {0, 60},
    {1, 60},
    {2, 60},
    {3, 60},
    {10, 60}
  };
}

void GamepadMidiController::buttonPressed(int button, double velocity)
{
  if (!m_noteMap.count(button)) {
    return;
  }

  
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;

  int pitch = m_noteMap[button];
  sequencer->OnInputEvent(NoteEventType::noteOn, pitch, velocity);
}

void GamepadMidiController::buttonReleased(int button, double velocity)
{
  if (!m_noteMap.count(button)) {
    return;
  }

  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;

  int pitch = m_noteMap[button];
  sequencer->OnInputEvent(NoteEventType::noteOff, pitch, velocity);
}
} // namespace dgk
