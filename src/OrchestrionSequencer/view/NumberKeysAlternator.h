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

#include "OrchestrionSequencer/IOrchestrionSequencer.h"

namespace dgk
{
//! Maps automatic-playback hand note events to the schematic number keys of
//! NumberKeysAnimation: each hand alternates between its two finger keys
//! (left middle/index on 2/3, right index/middle on 8/9). 0 = no key pressed.
class NumberKeysAlternator
{
public:
  void reset()
  {
    m_leftKey = 0;
    m_rightKey = 0;
    m_leftNextIsSecond = false;
    m_rightNextIsSecond = false;
  }

  void onEvent(const AutoPlayEvent &event)
  {
    constexpr int leftKeyA = 2;  // left middle finger
    constexpr int leftKeyB = 3;  // left index finger
    constexpr int rightKeyA = 8; // right index finger
    constexpr int rightKeyB = 9; // right middle finger

    if (event.isLeftHand)
    {
      if (event.type == NoteEventType::noteOn)
      {
        m_leftKey = m_leftNextIsSecond ? leftKeyB : leftKeyA;
        m_leftNextIsSecond = !m_leftNextIsSecond;
      }
      else
        m_leftKey = 0;
    }
    else
    {
      if (event.type == NoteEventType::noteOn)
      {
        m_rightKey = m_rightNextIsSecond ? rightKeyB : rightKeyA;
        m_rightNextIsSecond = !m_rightNextIsSecond;
      }
      else
        m_rightKey = 0;
    }
  }

  int leftKey() const { return m_leftKey; }
  int rightKey() const { return m_rightKey; }

private:
  int m_leftKey = 0;
  int m_rightKey = 0;
  bool m_leftNextIsSecond = false;
  bool m_rightNextIsSecond = false;
};
} // namespace dgk
