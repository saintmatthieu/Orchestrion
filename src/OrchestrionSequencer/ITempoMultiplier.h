/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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

#include "framework/global/async/notification.h"
#include "framework/global/modularity/ioc.h"

namespace dgk
{
/**
 * How much faster than its tempo markings the score is played: by the
 * automatic player, and as the tempo the performer is first expected at. A
 * property of the score, saved with it: changing it is an edit of the score,
 * undoable and asking to be saved like any other.
 */
class ITempoMultiplier : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(ITempoMultiplier);

public:
  virtual ~ITempoMultiplier() = default;

  static constexpr double min = 0.25;
  static constexpr double max = 3.0;

  /** 1 for the tempo as marked. */
  virtual double multiplier() const = 0;
  /** Clamped to [min, max]. */
  virtual void setMultiplier(double) = 0;

  /** The multiplier changed: another score, an edit, an undo. */
  virtual muse::async::Notification multiplierChanged() const = 0;
};
} // namespace dgk
