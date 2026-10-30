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
#include "OrchestrionShortcutsResolver.h"

#include <algorithm>

namespace dgk
{
namespace
{
// The scopes of shortcuts.json
constexpr auto navigationScope = "NAVIGATION";
constexpr auto playbackScope = "PLAYBACK";
constexpr auto notationPageScope = "NOTATION_PAGE";
} // namespace

muse::shortcuts::Shortcut OrchestrionShortcutsResolver::selectActive(
    const muse::shortcuts::ShortcutList &list) const
{
  if (list.empty())
    return {};
  const auto best =
      std::max_element(list.begin(), list.end(),
                       [this](const muse::shortcuts::Shortcut &a,
                              const muse::shortcuts::Shortcut &b) {
                         return scopePriority(a.scope) < scopePriority(b.scope);
                       });
  return *best;
}

muse::TranslatableString
OrchestrionShortcutsResolver::scopeTitle(const std::string &scopeCode) const
{
  return muse::TranslatableString::untranslatable(
      muse::String::fromStdString(scopeCode));
}

int OrchestrionShortcutsResolver::scopePriority(const std::string &scope) const
{
  // Navigation's keys (Tab, Esc) work anytime, but its arrows and Space only
  // take over from Orchestrion's while a control has the keyboard focus.
  if (scope == navigationScope)
    return isKeyboardNavigating() ? 10 : 1;
  if (scope == playbackScope)
    return 2;
  if (scope == notationPageScope)
    return 3;
  return 0;
}

bool OrchestrionShortcutsResolver::isKeyboardNavigating() const
{
  // Highlight is on from the first navigation key until a mouse press, and
  // Esc leaves the control inactive.
  return navigationController()->isHighlight() &&
         navigationController()->activeControl() != nullptr;
}
} // namespace dgk
