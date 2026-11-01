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

#include <modularity/imoduleinterface.h>

#include <QPointF>
#include <QString>
#include <functional>
#include <optional>

namespace dgk
{
/**
 * What the score view paints that a demo can point at, which isn't a control
 * of its own: the staff-mode selector's arrows, say (see OrchestrionShell's
 * DemoCursor).
 */
class IScoreTargets : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IScoreTargets);

public:
  virtual ~IScoreTargets() = default;

  /**
   * Where a target is, in the window's coordinates, wherever it is scrolled
   * to, inside the view or not; nothing if there is no such target, or no
   * score. The targets:
   * - "Cylinder": the left cylinder's outer half, where the pointer makes it
   *   roll in to uncover the staves' modes;
   * - "StaffMode/<staff>/<mode|previous|next>": a staff's mode selector
   *   (staff 0 the upper one): its sketch of the mode, its arrows, which
   *   respond only while uncovered.
   */
  virtual std::optional<QPointF> position(const QString &target) const = 0;

  using Locator = std::function<std::optional<QPointF>(const QString &)>;

  /**
   * The score view's: where its targets are; none, null.
   */
  virtual void setLocator(Locator) = 0;
};
} // namespace dgk
