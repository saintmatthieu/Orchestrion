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
#include "ComputerKeyboardGestureController.h"

namespace dgk
{
ComputerKeyboardGestureController::ComputerKeyboardGestureController()
{
  keyboard()->keyPressed().onReceive(this, [this](char letter)
                                     { keyPressed(letter); });
  keyboard()->keyReleased().onReceive(this, [this](char letter)
                                      { keyReleased(letter); });
}

void ComputerKeyboardGestureController::keyPressed(char letter)
{
  letter = std::tolower(letter);

  if (std::find_if(m_noteMap.begin(), m_noteMap.end(),
                   [letter](const auto &note)
                   { return note.first == letter; }) == m_noteMap.end())
    return;

  if (m_pressedLetters.count(letter) > 0)
    // An auto-repeat event probably, the user keeping the finger pressed on
    // a key.
    return;

  m_pressedLetters.insert(letter);
  m_noteOn.send(m_noteMap.at(letter), std::optional<float>{});
}

void ComputerKeyboardGestureController::keyReleased(char letter)
{
  letter = std::tolower(letter);

  if (std::find_if(m_noteMap.begin(), m_noteMap.end(),
                   [letter](const auto &note)
                   { return note.first == letter; }) == m_noteMap.end())
    return;

  m_pressedLetters.erase(letter);
  m_noteOff.send(m_noteMap.at(letter));
}

muse::async::Channel<int, std::optional<float>>
ComputerKeyboardGestureController::noteOn() const
{
  return m_noteOn;
}

muse::async::Channel<int> ComputerKeyboardGestureController::noteOff() const
{
  return m_noteOff;
}
} // namespace dgk