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

#include "IIntensityController.h"

namespace dgk
{
/**
 * @brief Interprets the forward/backward inclination of the user's head as a
 * measure of intensity.
 * 
 * @details Uses openCV to capture the user's face and interpret the inclination
 * of the head as a measure of intensity.
 * 
 */
class WebcamBasedIntensityController : public IIntensityController
{
public:
  void init();

  muse::async::Channel<float> NewIntensity() const override;

private:
  muse::async::Channel<float> m_newIntensity;
};
} // namespace dgk