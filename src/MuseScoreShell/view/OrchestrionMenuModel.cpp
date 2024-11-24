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

  orchestrionUiActions()
      ->settableDevicesChanged(DeviceType::MidiController)
      .onNotify(this,
                [this]
                {
                  updateMenuItems(orchestrionUiActions()->settableDevices(
                                      DeviceType::MidiController),
                                  actionIds::chooseMidiControllerSubmenu);
                });

  orchestrionUiActions()
      ->settableDevicesChanged(DeviceType::PlaybackDevice)
      .onNotify(this,
                [this]
                {
                  updateMenuItems(orchestrionUiActions()->settableDevices(
                                      DeviceType::PlaybackDevice),
                                  actionIds::choosePlaybackDeviceSubmenu);
                });

  orchestrionUiActions()
      ->selectedDeviceChanged(DeviceType::MidiController)
      .onReceive(this,
                 [this](const std::string &deviceId)
                 {
                   selectMenuItem(actionIds::chooseMidiControllerSubmenu,
                                  deviceId);
                 });

  orchestrionUiActions()
      ->selectedDeviceChanged(DeviceType::PlaybackDevice)
      .onReceive(this,
                 [this](const std::string &deviceId)
                 {
                   selectMenuItem(actionIds::choosePlaybackDeviceSubmenu,
                                  deviceId);
                 });

  muse::uicomponents::MenuItemList items{makeFileMenu(), makeAudioMidiMenu()};
  setItems(items);
  selectMenuItem(
      actionIds::chooseMidiControllerSubmenu,
      orchestrionUiActions()->selectedDevice(DeviceType::MidiController));
  selectMenuItem(
      actionIds::choosePlaybackDeviceSubmenu,
      orchestrionUiActions()->selectedDevice(DeviceType::PlaybackDevice));
}

void OrchestrionMenuModel::selectMenuItem(const char *submenuId,
                                          const std::string &deviceId)
{
  using namespace muse::uicomponents;
  const QList<MenuItem *> subitems = findItem(QString{submenuId}).subitems();
  std::for_each(subitems.begin(), subitems.end(),
                [](MenuItem *item) { item->setSelected(false); });
  const auto it = std::find_if(
      subitems.begin(), subitems.end(), [deviceId](const MenuItem *item)
      { return item->args().arg<std::string>(1) == deviceId; });
  IF_ASSERT_FAILED(it != subitems.end()) return;
  (*it)->setSelected(true);
}

void OrchestrionMenuModel::updateMenuItems(
    const std::vector<DeviceAction> &devices, const std::string &menuId)
{
  using namespace muse::uicomponents;
  auto &menu = findItem(QString::fromStdString(menuId));
  IF_ASSERT_FAILED(menu.isValid()) return;
  menu.setSubitems(getMenuItems(devices));
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

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeAudioMidiSubmenu(DeviceType deviceType)
{
  auto subenu = makeMenuItem(deviceType == DeviceType::MidiController
                                 ? actionIds::chooseMidiControllerSubmenu
                                 : actionIds::choosePlaybackDeviceSubmenu);
  if (subenu)
  {
    subenu->setTitle(muse::TranslatableString(
        "appshell/menu/audio-midi", deviceType == DeviceType::MidiController
                                        ? "&MIDI controller"
                                        : "&Playback device"));
    subenu->setSubitems(
        getMenuItems(orchestrionUiActions()->settableDevices(deviceType)));
  }
  return subenu;
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeAudioMidiMenu()
{
  using namespace muse::uicomponents;
  return makeMenu(
      muse::TranslatableString("appshell/menu/audio-midi", "&Audio/MIDI"),
      {makeAudioMidiSubmenu(DeviceType::MidiController),
       makeAudioMidiSubmenu(DeviceType::PlaybackDevice)},
      "menu-audio-midi");
}

QList<muse::uicomponents::MenuItem *>
OrchestrionMenuModel::getMenuItems(const std::vector<DeviceAction> &devices)
{
  using namespace muse::uicomponents;
  QList<MenuItem *> menu;
  std::for_each(devices.begin(), devices.end(),
                [this, &menu](const DeviceAction &action)
                {
                  auto item = makeMenuItem(action.id);
                  IF_ASSERT_FAILED(item) return;
                  item->setTitle(muse::TranslatableString::untranslatable(
                      muse::String::fromStdString(action.deviceName)));
                  item->setArgs(
                      muse::actions::ActionData::make_arg2<std::string>(
                          action.id, action.deviceId));
                  item->setSelectable(true);
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