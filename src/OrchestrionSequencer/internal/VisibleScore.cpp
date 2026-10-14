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
#include "VisibleScore.h"

namespace dgk
{
std::optional<std::pair<int, int>> VisibleScore::visibleTicks() const
{
  return m_ticks;
}

void VisibleScore::setVisibleTicks(int begin, int end)
{
  const std::pair<int, int> ticks{begin, end};
  if (m_ticks == ticks)
    return;
  m_ticks = ticks;
  m_changed.notify();
}

muse::async::Notification VisibleScore::visibleTicksChanged() const
{
  return m_changed;
}
} // namespace dgk
