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

namespace dgk
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

  setItems({makeFileMenu(), makeAudioMidiMenu(), makeKeyboardMenu()});

  computerKeyboard()->layoutChanged().onNotify(
      this, [this] { updateSelectedKeyboardMenuItem(); });
  updateSelectedKeyboardMenuItem();

  midiControllerManager()->trySelectDefaultDevice();
  midiSynthesizerManager()->trySelectDefaultDevice();
  playbackDeviceManager()->trySelectDefaultDevice();

  for (const auto &[deviceType, menuId] : actionIds::chooseDevicesSubmenu)
  {
    orchestrionUiActions()
        ->settableDevicesChanged(deviceType)
        .onNotify(this,
                  [this, deviceType, menuId]
                  {
                    updateMenuItems(
                        orchestrionUiActions()->settableDevices(deviceType),
                        menuId);
                  });

    orchestrionUiActions()
        ->selectedDeviceChanged(deviceType)
        .onReceive(this, [this, deviceType, menuId](const std::string &deviceId)
                   { selectMenuItem(menuId, deviceId); });

    if (const auto deviceId =
            orchestrionUiActions()->selectedDevice(deviceType);
        !deviceId.empty())
      selectMenuItem(menuId, deviceId);
  }
}

void OrchestrionMenuModel::updateSelectedKeyboardMenuItem()
{
  using namespace muse::uicomponents;
  const auto layout = computerKeyboard()->layout();
  const std::unordered_map<IComputerKeyboard::Layout, std::string> ids =
      uiActions()->computerKeyboardSetterActionIds();
  IF_ASSERT_FAILED(ids.find(layout) != ids.end()) return;
  const std::string id = ids.at(layout);
  const QList<MenuItem *> subitems =
      findItem(QString{"menu-keyboard"}).subitems();
  std::for_each(subitems.begin(), subitems.end(), [&](MenuItem *item)
                { item->setSelected(item->id().toStdString() == id); });
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
  if (it == subitems.end())
    return;
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
  muse::uicomponents::MenuItemList fileItems{makeMenuItem("file-open"),
                                             makeMenuItem("file-close")};
  return makeMenu(muse::TranslatableString("appshell/menu/file", "&File"),
                  fileItems, "menu-file");
}

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeAudioMidiSubmenu(DeviceType deviceType)
{
  auto subenu = makeMenuItem(actionIds::chooseDevicesSubmenu.at(deviceType));
  if (subenu)
  {
    subenu->setTitle(muse::TranslatableString(
        "appshell/menu/audio-midi",
        deviceType == DeviceType::MidiController    ? "&MIDI controller"
        : deviceType == DeviceType::MidiSynthesizer ? "MIDI &synthesizer"
                                                    : "&Playback device"));
    subenu->setSubitems(
        getMenuItems(orchestrionUiActions()->settableDevices(deviceType)));
  }
  return subenu;
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeKeyboardMenu()
{
  QList<muse::uicomponents::MenuItem *> menu;
  for (const auto [layout, id] : uiActions()->computerKeyboardSetterActionIds())
  {
    auto item = makeMenuItem(id);
    IF_ASSERT_FAILED(item) return nullptr;
    item->setTitle(
        muse::TranslatableString::untranslatable(muse::String::fromStdString(
            IComputerKeyboard::layoutToString(layout))));
    item->setSelectable(true);
    menu.append(item);
  }
  return makeMenu(
      muse::TranslatableString("appshell/menu/keyboard", "&Keyboard"), menu,
      "menu-keyboard");
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeAudioMidiMenu()
{
  using namespace muse::uicomponents;
  return makeMenu(
      muse::TranslatableString("appshell/menu/audio-midi", "&Audio/MIDI"),
      {makeAudioMidiSubmenu(DeviceType::MidiController),
       makeAudioMidiSubmenu(DeviceType::MidiSynthesizer),
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
} // namespace dgk