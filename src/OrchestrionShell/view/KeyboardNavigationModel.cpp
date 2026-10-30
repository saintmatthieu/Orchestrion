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
#include "KeyboardNavigationModel.h"

namespace dgk
{
KeyboardNavigationModel::KeyboardNavigationModel(QObject *parent)
    : QObject(parent)
{
}

void KeyboardNavigationModel::load()
{
  const auto onChange = [this]
  {
    if (navigationController()->isHighlight() &&
        navigationController()->activeControl())
      emit navigated();
  };
  navigationController()->navigationChanged().onNotify(this, onChange);
  // The first key moves the focus before turning the highlight on.
  navigationController()->highlightChanged().onNotify(this, onChange);
}

void KeyboardNavigationModel::release()
{
  if (!navigationController()->isHighlight())
    return;
  // Not resetNavigation(), which makes the first control active again: the
  // next Tab would then go on from it, rather than start with it.
  for (muse::ui::INavigation *const nav :
       std::initializer_list<muse::ui::INavigation *>{
           navigationController()->activeControl(),
           navigationController()->activePanel(),
           navigationController()->activeSection()})
    if (nav)
      nav->setActive(false);
  navigationController()->setIsHighlight(false);
}
} // namespace dgk
