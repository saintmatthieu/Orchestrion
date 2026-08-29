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
#include "GestureInput.h"
#include "ComputerKeyboard/ComputerKeyboardGestureController.h"
#include "MidiDevice/MidiDeviceGestureController.h"

namespace dgk
{
void GestureInput::init()
{
  m_controllers.push_back(
      std::make_unique<ComputerKeyboardGestureController>());
  m_controllers.push_back(std::make_unique<MidiDeviceGestureController>());

  for (const auto &controller : m_controllers)
  {
    controller->noteOn().onReceive(
        this, [this](int pitch, std::optional<float> velocity)
        { m_noteOn.send(pitch, std::move(velocity)); });
    controller->noteOff().onReceive(this, [this](int pitch)
                                    { m_noteOff.send(pitch); });
  }
}

muse::async::Channel<int, std::optional<float>> GestureInput::noteOn() const
{
  return m_noteOn;
}

muse::async::Channel<int> GestureInput::noteOff() const { return m_noteOff; }
} // namespace dgk
