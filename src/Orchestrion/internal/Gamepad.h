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
#include <QObject>
#include <modularity/ioc.h>

namespace dgk
{

class GamepadImpl;

class Gamepad : public IGamepad, public muse::Injectable
{
public:
    Gamepad();
    ~Gamepad();
    void onButtonPressed(int, double);
    void onButtonReleased(int, double);

private:
    muse::async::Channel<int, double> buttonPressed() const override;
    muse::async::Channel<int, double> buttonReleased() const override;

    muse::async::Channel<int, double> m_buttonPressed;
    muse::async::Channel<int, double> m_buttonReleased;

    std::shared_ptr<GamepadImpl> m_impl;

};

}
