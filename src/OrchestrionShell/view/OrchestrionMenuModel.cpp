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
#include "OrchestrionMenuModel.h"
#include "types/translatablestring.h"

namespace dgk::orchestrion
{
OrchestrionMenuModel::OrchestrionMenuModel(QObject *parent)
    : AbstractMenuModel(parent)
{
}

QWindow *OrchestrionMenuModel::appWindow() const { return m_appWindow; }

void OrchestrionMenuModel::setAppWindow(QWindow *appWindow)
{
  m_appWindow = appWindow;
}

void OrchestrionMenuModel::load()
{
}

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeMenuItem(const muse::actions::ActionCode &actionCode,
                                   muse::uicomponents::MenuItemRole menuRole)
{
  auto *item = makeMenuItem(actionCode);
  item->setRole(menuRole);
  return item;
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeFileMenu()
{
  muse::uicomponents::MenuItemList fileItems{makeMenuItem("file-open")};
  return makeMenu(mu::TranslatableString("appshell/menu/file", "&File"),
                  fileItems, "menu-file");
}

QRect OrchestrionMenuModel::appMenuAreaRect() const
{
  return m_appMenuAreaRect;
}

void OrchestrionMenuModel::setAppMenuAreaRect(QRect appMenuAreaRect)
{
  if (m_appMenuAreaRect == appMenuAreaRect)
    return;

  m_appMenuAreaRect = appMenuAreaRect;
  emit appMenuAreaRectChanged(m_appMenuAreaRect);
}

QRect OrchestrionMenuModel::openedMenuAreaRect() const
{
  return m_openedMenuAreaRect;
}

void OrchestrionMenuModel::setOpenedMenuAreaRect(QRect openedMenuAreaRect)
{
  if (m_openedMenuAreaRect == openedMenuAreaRect)
    return;

  m_openedMenuAreaRect = openedMenuAreaRect;
  emit openedMenuAreaRectChanged(m_openedMenuAreaRect);
}
} // namespace dgk::orchestrion