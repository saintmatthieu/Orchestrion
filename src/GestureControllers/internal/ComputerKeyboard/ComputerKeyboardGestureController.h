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

#include "IComputerKeyboard.h"
#include "IGestureController.h"
#include <async/asyncable.h>
#include <memory>
#include <modularity/ioc.h>
#include <unordered_map>
#include <unordered_set>

namespace dgk
{
class ComputerKeyboardGestureController : public IGestureController,
                                          public muse::Injectable,
                                          public muse::async::Asyncable
{
public:
  ComputerKeyboardGestureController();

  static bool isFunctional() { return true; }

private:
  struct Note
  {
    const int pitch;
    const float velocity;
  };

  void keyPressed(char);
  void keyReleased(char);
  void updateNoteMap();

  muse::async::Channel<int, float> noteOn() const override;
  muse::async::Channel<int> noteOff() const override;

  muse::Inject<IComputerKeyboard> keyboard;

  std::unordered_set<char> m_pressedLetters;
  std::unordered_map<char, Note> m_noteMap;

  muse::async::Channel<int, float> m_noteOn;
  muse::async::Channel<int> m_noteOff;
};

} // namespace dgk