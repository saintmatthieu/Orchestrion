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

#include <optional>
#include <utility>

namespace dgk
{
/**
 * The part of the score the performer sees. Playing the notes, the performer
 * restarts within it only.
 */
class IVisibleScore : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IVisibleScore);

public:
  virtual ~IVisibleScore() = default;

  /** In ticks without repeats, as [begin, end); none while it is unknown. */
  virtual std::optional<std::pair<int, int>> visibleTicks() const = 0;
  virtual void setVisibleTicks(int begin, int end) = 0;
  virtual muse::async::Notification visibleTicksChanged() const = 0;
};
} // namespace dgk
