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

#include "IGestureController.h"
#include "ITouchpad.h"

#include <async/asyncable.h>
#include <modularity/ioc.h>

#include <memory>
#include <optional>

namespace dgk
{
class SwipeGestureController : public IGestureController,
                               public muse::Injectable,
                               public muse::async::Asyncable
{
public:
  SwipeGestureController(const ITouchpad &touchpad);

  static bool isFunctional() { return false; }

private:
  muse::async::Channel<int, float> noteOn() const override;
  muse::async::Channel<int> noteOff() const override;

  muse::async::Channel<int, float> m_noteOn;
  muse::async::Channel<int> m_noteOff;

  enum class Direction
  {
    up,
    down
  };

  struct Finger
  {
    Finger(int id, float y) : id{id}, prevY{y} {}
    const int id;
    float prevY;
    bool swiping = false;
    int swipingFor = 0; // as in "has been swiping for this many samples"
    float swipeStartY = 0.0f;
    std::optional<Direction> triggeringDirection;
  };
  std::optional<Finger> m_left;
  std::optional<Finger> m_right;
};
} // namespace dgk