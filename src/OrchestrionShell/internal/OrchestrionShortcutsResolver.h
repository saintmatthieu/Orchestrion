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

#include "OrchestrionCommon/OrchestrionIoc.h"

#include <shortcuts_v2/ishortcutsresolver.h>
#include <ui/inavigationcontroller.h>

namespace dgk
{
/**
 * Chooses between the commands bound to the same keys (shortcuts.json) by
 * their scope. Orchestrion's keys win (Space plays, the arrows step through
 * the score), except in keyboard navigation: once Tab has focused a control,
 * the arrows move between controls and Space or Enter presses the focused one,
 * until Esc or a mouse click leaves it.
 */
class OrchestrionShortcutsResolver : public muse::shortcuts::IShortcutsResolver,
                                     public dgk::Injectable
{
  dgk::Inject<muse::ui::INavigationController> navigationController{this};

public:
  muse::shortcuts::Shortcut
  selectActive(const muse::shortcuts::ShortcutList &list) const override;
  muse::TranslatableString
  scopeTitle(const std::string &scopeCode) const override;

private:
  int scopePriority(const std::string &scope) const;
  bool isKeyboardNavigating() const;
};
} // namespace dgk
