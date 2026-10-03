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

#include <vector>

namespace dgk
{
/**
 * How a staff of the played part is played.
 */
enum class StaffMode
{
  /** By the performer, any key striking the staff's next chord. */
  rhythm,
  /**
   * By the machine, following the performer's position and tempo on the
   * other staves.
   */
  follow,
  /** Not at all: silent. */
  mute,
};

/**
 * The mode of each staff of the played part (the one the sequencer plays;
 * staff 0 its upper — right-hand — staff, 1 its lower). A property of the
 * score, saved with it: changing it is an edit of the score, undoable and
 * asking to be saved like any other.
 */
class IStaffModes : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IStaffModes);

public:
  virtual ~IStaffModes() = default;

  /** The staff's mode: Rhythm for a staff the score says nothing of. */
  virtual StaffMode mode(int staff) const = 0;
  virtual void setMode(int staff, StaffMode) = 0;

  /** The modes a staff can be set to, in the order they are offered in. */
  virtual std::vector<StaffMode> availableModes() const = 0;

  /** The modes changed: another score, an edit, an undo. */
  virtual muse::async::Notification modesChanged() const = 0;
};
} // namespace dgk
