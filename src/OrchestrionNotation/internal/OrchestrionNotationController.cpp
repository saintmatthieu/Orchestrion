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
#include "OrchestrionNotationController.h"
#include "engraving/dom/masterscore.h"
#include "log.h"

namespace dgk::orchestrion
{
void OrchestrionNotationController::configureNotation()
{
  dispatcher()->dispatch("view-mode-continuous");
  m_pageSizeChanged.notify();
}

muse::async::Notification OrchestrionNotationController::pageSizeChanged() const
{
  return m_pageSizeChanged;
}

mu::Size OrchestrionNotationController::pageSize() const
{
  const auto notation = globalContext()->currentMasterNotation();
  IF_ASSERT_FAILED(notation) { return {}; }
  const auto masterScore = notation->masterScore();
  IF_ASSERT_FAILED(masterScore) { return {}; }
  const auto dpi = uiConfiguration()->logicalDpi();
  const auto inches = scoreRenderer()->pageSizeInch(masterScore->score());
  return mu::Size{static_cast<int>(inches.width() * dpi),
                  static_cast<int>(inches.height() * dpi)};
}
} // namespace dgk::orchestrion