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

#include <algorithm>
#include <cctype>

namespace dgk
{
ComputerKeyboardGestureController::ComputerKeyboardGestureController(
    Dispatch dispatch)
    : m_keyboard{keyboard()}, m_dispatch{std::move(dispatch)}
{
  m_keyboard->setReceiver(
      [this](char letter, bool pressed)
      { pressed ? keyPressed(letter) : keyReleased(letter); });
}

ComputerKeyboardGestureController::~ComputerKeyboardGestureController()
{
  // The keys keep coming until the keyboard is gone.
  m_keyboard->setReceiver(nullptr);
}

void ComputerKeyboardGestureController::keyPressed(char letter)
{
  letter = std::tolower(letter);

  if (std::find_if(m_noteMap.begin(), m_noteMap.end(),
                   [letter](const auto &note)
                   { return note.first == letter; }) == m_noteMap.end())
    return;

  {
    std::scoped_lock lock{m_mutex};
    if (!m_pressedLetters.insert(letter).second)
      // An auto-repeat event probably, the user keeping the finger pressed on
      // a key.
      return;
  }
  m_dispatch({IGestureInput::Gesture::Kind::noteOn, m_noteMap.at(letter),
              std::nullopt, false});
}

void ComputerKeyboardGestureController::keyReleased(char letter)
{
  letter = std::tolower(letter);

  if (std::find_if(m_noteMap.begin(), m_noteMap.end(),
                   [letter](const auto &note)
                   { return note.first == letter; }) == m_noteMap.end())
    return;

  {
    std::scoped_lock lock{m_mutex};
    if (m_pressedLetters.erase(letter) == 0)
      // Not pressed: a release told twice, or of a key pressed before
      return;
  }
  m_dispatch({IGestureInput::Gesture::Kind::noteOff, m_noteMap.at(letter),
              std::nullopt, false});
}
} // namespace dgk
