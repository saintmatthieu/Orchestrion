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
#include "OrchestrionActionIds.h"
#include "log.h"
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

void OrchestrionMenuModel::setOpenedMenuId(QString openedMenuId)
{
  if (m_openedMenuId == openedMenuId)
    return;

  m_openedMenuId = openedMenuId;
  emit openedMenuIdChanged(m_openedMenuId);
}

void OrchestrionMenuModel::load()
{
  AbstractMenuModel::load();

  orchestrionUiActions()->settablePlaybackDevicesChanged().onNotify(
      this, [this]() { updatePlaybackDeviceMenuItems(); });

  muse::uicomponents::MenuItemList items{makeFileMenu(), makeAudioMidiMenu()};
  setItems(items);
}

void OrchestrionMenuModel::updatePlaybackDeviceMenuItems()
{
  using namespace muse::uicomponents;
  auto &menu = findItem(QString{actionIds::choosePlaybackDeviceSubmenu});
  IF_ASSERT_FAILED(menu.isValid()) return;
  menu.setSubitems(getPlaybackDeviceMenuItems());
  emit itemChanged(&menu);
}

QString OrchestrionMenuModel::openedMenuId() const { return m_openedMenuId; }

void OrchestrionMenuModel::openMenu(const QString &menuId, bool byHover)
{
  emit openMenuRequested(menuId, byHover);
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeFileMenu()
{
  muse::uicomponents::MenuItemList fileItems{makeMenuItem("file-open")};
  return makeMenu(muse::TranslatableString("appshell/menu/file", "&File"),
                  fileItems, "menu-file");
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeAudioMidiMenu()
{
  using namespace muse::uicomponents;
  auto menu = makeMenuItem(actionIds::choosePlaybackDeviceSubmenu);
  IF_ASSERT_FAILED(menu) return nullptr;
  menu->setTitle(
      muse::TranslatableString("appshell/menu/audio-midi", "Playback device"));
  menu->setSubitems(getPlaybackDeviceMenuItems());

  return makeMenu(
      muse::TranslatableString("appshell/menu/audio-midi", "&Audio/MIDI"),
      {menu}, "menu-audio-midi");
}

QList<muse::uicomponents::MenuItem *>
OrchestrionMenuModel::getPlaybackDeviceMenuItems()
{
  using namespace muse::uicomponents;
  QList<MenuItem *> menu;
  const std::vector<DeviceAction> devices =
      orchestrionUiActions()->settablePlaybackDevices();
  std::for_each(
      devices.begin(), devices.end(),
      [this, &menu](const DeviceAction &device)
      {
        auto item = makeMenuItem(device.id);
        IF_ASSERT_FAILED(item) return;
        item->setTitle(muse::TranslatableString::untranslatable(
            muse::String::fromStdString(device.name)));
        item->setArgs(
            muse::actions::ActionData::make_arg1<std::string>(device.id));
        menu.append(item);
      });
  return menu;
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