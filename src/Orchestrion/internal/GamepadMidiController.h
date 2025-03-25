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

#include "IGamepad.h"
#include "IGamepadMidiController.h"
#include "IOrchestrion.h"
#include <async/asyncable.h>
#include <memory>
#include <modularity/ioc.h>

namespace dgk
{
class GamepadMidiController : public IGamepadMidiController,
                              public muse::Injectable,
                              public muse::async::Asyncable
{
  muse::Inject<IOrchestrion> orchestrion = {this};
  muse::Inject<IGamepad> gamepad = {this};

public:
  void init();

private:
  struct Note
  {
    const int pitch;
    const float velocity;
  };

  void buttonPressed(int button, double velocity) override;
  void buttonReleased(int button, double velocity) override;
  void updateNoteMap();

  std::unordered_map<int, int> m_noteMap;
};

} // namespace dgk
