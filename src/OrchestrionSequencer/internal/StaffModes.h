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

#include "IStaffModes.h"
#include "OrchestrionCommon/OrchestrionIoc.h"

#include <async/asyncable.h>
#include <project/iprojectsession.h>

namespace dgk
{
/**
 * The staff modes, kept in a meta tag of the score — which MusicXML carries
 * as a `<miscellaneous-field>` of its `<identification>`, and MuseScore files
 * natively — and changed through the score's undo stack.
 */
class StaffModes : public IStaffModes,
                   public muse::async::Asyncable,
                   public dgk::Injectable
{
  dgk::Inject<mu::project::IProjectSession> projectSession{this};

public:
  void init();

  StaffMode mode(int staff) const override;
  void setMode(int staff, StaffMode) override;
  std::vector<StaffMode> availableModes() const override;
  muse::async::Notification modesChanged() const override;

private:
  /** What the score says, a staff it says nothing of left out. */
  std::vector<StaffMode> savedModes() const;
  /** Notifies if what mode() returns changed. */
  void checkChanged();

  std::vector<StaffMode> m_modes;
  muse::async::Notification m_modesChanged;
};
} // namespace dgk
