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

#include <QDir>

namespace dgk
{
namespace
{
constexpr auto audioMidiMenuId = "menu-audio-midi";
constexpr auto keyboardMenuId = "menu-keyboard";
constexpr auto toggleRecordingMenuId = "orchestrion-advanced-toggle-recording";
constexpr auto toggleNoteInfoMenuId = "orchestrion-advanced-toggle-note-info";
constexpr auto toggleTempoVizMenuId =
    "orchestrion-advanced-toggle-tempo-visualization";
constexpr auto toggleTimingFeedbackMenuId =
    "orchestrion-advanced-toggle-timing-feedback";
constexpr auto togglePersistentTimingMarksMenuId =
    "orchestrion-advanced-toggle-persistent-timing-marks";
constexpr auto toggleHandSyncScoreMenuId =
    "orchestrion-advanced-toggle-hand-sync-score";
constexpr auto toggleDynamicsScoreMenuId =
    "orchestrion-advanced-toggle-dynamics-score";
} // namespace

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

void OrchestrionMenuModel::createMenus(bool velocityRecordingEnabled)
{
  setItems({makeFileMenu(velocityRecordingEnabled), makeViewMenu(),
            makeAudioMidiMenu(), makeAdvancedMenu(velocityRecordingEnabled),
            makeHelpMenu()});
}

void OrchestrionMenuModel::load()
{
  AbstractMenuModel::load();

  createMenus(sequencerConfiguration()->velocityRecordingEnabled());

  sequencerConfiguration()->velocityRecordingEnabledChanged().onNotify(
      this,
      [this]
      {
        const auto recordingEnabled =
            sequencerConfiguration()->velocityRecordingEnabled();
        createMenus(recordingEnabled);
      });

  synthesisConfiguration()->reverbPresetChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->noteInfoTooltipEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->tempoVisualizationEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->timingFeedbackEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->persistentTimingMarksEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->handSyncScoreEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->dynamicsScoreEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  for (const auto &[deviceType, menuId] : actionIds::chooseDevicesSubmenu)
  {
    orchestrionUiActions()
        ->settableDevicesChanged(deviceType)
        .onNotify(
            this,
            [this, deviceType, menuId]
            {
              updateMenuItems(
                  orchestrionUiActions()->settableDevices(deviceType), menuId);
              selectMenuItem(
                  menuId, orchestrionUiActions()->selectedDevice(deviceType));
            });

    orchestrionUiActions()
        ->selectedDeviceChanged(deviceType)
        .onReceive(this, [this, deviceType, menuId](const std::string &deviceId)
                   { selectMenuItem(menuId, deviceId); });

    switch (deviceType)
    {
    case DeviceType::MidiController:
      midiDeviceService()->selectedDeviceChanged().onNotify(
          this,
          [this, deviceType, menuId]
          {
            const auto selectedDevice = midiDeviceService()->selectedDevice();
            if (selectedDevice)
              selectMenuItem(menuId, selectedDevice->value);
            else
              selectMenuItem(menuId, {});
          });
      break;
    case DeviceType::MidiSynthesizer:
    case DeviceType::PlaybackDevice:
      break;
    }

    if (const auto deviceId =
            orchestrionUiActions()->selectedDevice(deviceType);
        !deviceId.empty())
      selectMenuItem(menuId, deviceId);
  }
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
  if (menuId == audioMidiMenuId)
    for (auto deviceType : kDeviceTypes)
    {
      const auto menuId = actionIds::chooseDevicesSubmenu.at(deviceType);
      updateMenuItems(orchestrionUiActions()->settableDevices(deviceType),
                      menuId);
      selectMenuItem(menuId,
                     orchestrionUiActions()->selectedDevice(deviceType));
    }
  emit openMenuRequested(menuId, byHover);
}

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeFileMenu(bool withSaveItem)
{
  QList<muse::uicomponents::MenuItem *> menu{
      makeMenuItem("orchestrion-file-open",
                   muse::TranslatableString("appshell/menu/file",
                                            "Open from &computer…")),
      makeExampleScoresSubmenu(),
      makeSeparator(),
      makeMenuItem("orchestrion-search-musescore",
                   muse::TranslatableString("appshell/menu/file",
                                            "Search on &musescore.com")),
      makeSeparator(),
      makeMenuItem("orchestrion-file-help",
                   muse::TranslatableString("appshell/menu/file",
                                            "&Help me find scores…"))};

  if (withSaveItem)
  {
    menu.append(
        makeMenuItem("orchestrion-file-save",
                     muse::TranslatableString("appshell/menu/file", "Save")));
    menu.append(makeMenuItem(
        "orchestrion-file-save-as",
        muse::TranslatableString("appshell/menu/file", "Save &as…")));
  }

  return makeMenu(muse::TranslatableString("appshell/menu/file", "&File"), menu,
                  "menu-orchestrion-file");
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeViewMenu()
{
  QList<muse::uicomponents::MenuItem *> menu{makeMenuItem(
      "view-toggle-fullscreen",
      muse::TranslatableString("appshell/menu/view", "&Fullscreen"))};
  return makeMenu(muse::TranslatableString("appshell/menu/view", "&View"), menu,
                  "menu-orchestrion-view");
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeHelpMenu()
{
  QList<muse::uicomponents::MenuItem *> menu{
      makeMenuItem("orchestrion-help-number-keys",
                   muse::TranslatableString("appshell/menu/help",
                                            "How to play with the &keyboard"))};
  return makeMenu(muse::TranslatableString("appshell/menu/help", "&Help"), menu,
                  "menu-orchestrion-help");
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeExampleScoresSubmenu()
{
  using namespace muse::uicomponents;

  const auto scoreDir =
      globalConfiguration()->appDataPath().toQString() + "scores/";
  const QDir dir(scoreDir);
  const auto entries =
      dir.entryInfoList({"*.mscz", "*.mscx", "*.mxl", "*.musicxml", "*.xml"},
                        QDir::Files, QDir::Name);

  QList<MenuItem *> items;
  int index = 0;
  for (const auto &entry : entries)
  {
    const QString displayName = entry.completeBaseName().replace('_', ' ');

    auto *item = new MenuItem(this);
    item->setId(QString("example-score-%1").arg(index++));

    muse::ui::UiAction action;
    action.code = "orchestrion-file-open";
    action.title = muse::TranslatableString::untranslatable(
        muse::String::fromQString(displayName));
    item->setAction(action);

    muse::ui::UiActionState state;
    state.enabled = true;
    item->setState(state);

    item->setArgs(muse::actions::ActionData::make_arg2<QUrl, QString>(
        QUrl::fromLocalFile(entry.absoluteFilePath()), displayName));

    items.append(item);
  }

  return makeMenu(
      muse::TranslatableString("appshell/menu/file", "&Example scores"), items,
      "menu-orchestrion-example-scores");
}

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeAudioMidiSubmenu(DeviceType deviceType)
{
  auto subenu = makeMenuItem(actionIds::chooseDevicesSubmenu.at(deviceType));
  if (subenu)
  {
    subenu->setTitle(deviceType == DeviceType::MidiController
                         ? muse::TranslatableString("appshell/menu/audio-midi",
                                                    "&MIDI controller")
                     : deviceType == DeviceType::MidiSynthesizer
                         ? muse::TranslatableString("appshell/menu/audio-midi",
                                                    "MIDI &synthesizer")
                         : muse::TranslatableString("appshell/menu/audio-midi",
                                                    "&Playback device"));
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
       makeAudioMidiSubmenu(DeviceType::MidiSynthesizer),
       makeAudioMidiSubmenu(DeviceType::PlaybackDevice)},
      audioMidiMenuId);
}

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeAdvancedMenu(bool velocityRecordingEnabled)
{
  using namespace muse::uicomponents;
  muse::uicomponents::MenuItem *const item =
      makeMenuItem(toggleRecordingMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "&Toggle velocity recording"));
  item->setSelectable(true);
  item->setSelected(velocityRecordingEnabled);

  muse::uicomponents::MenuItem *const noteInfoItem =
      makeMenuItem(toggleNoteInfoMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "Show &note info on hover"));
  noteInfoItem->setSelectable(true);
  noteInfoItem->setSelected(sequencerConfiguration()->noteInfoTooltipEnabled());

  muse::uicomponents::MenuItem *const tempoVizItem =
      makeMenuItem(toggleTempoVizMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "Show &tempo visualization"));
  tempoVizItem->setSelectable(true);
  tempoVizItem->setSelected(
      sequencerConfiguration()->tempoVisualizationEnabled());

  muse::uicomponents::MenuItem *const timingFeedbackItem =
      makeMenuItem(toggleTimingFeedbackMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "Show timing &feedback"));
  timingFeedbackItem->setSelectable(true);
  timingFeedbackItem->setSelected(
      sequencerConfiguration()->timingFeedbackEnabled());

  muse::uicomponents::MenuItem *const persistentMarksItem =
      makeMenuItem(togglePersistentTimingMarksMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "&Keep timing marks on screen"));
  persistentMarksItem->setSelectable(true);
  persistentMarksItem->setSelected(
      sequencerConfiguration()->persistentTimingMarksEnabled());

  muse::uicomponents::MenuItem *const handSyncScoreItem =
      makeMenuItem(toggleHandSyncScoreMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "Score hand s&ynchronization"));
  handSyncScoreItem->setSelectable(true);
  handSyncScoreItem->setSelected(
      sequencerConfiguration()->handSyncScoreEnabled());

  muse::uicomponents::MenuItem *const dynamicsScoreItem =
      makeMenuItem(toggleDynamicsScoreMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "Score dynamics s&moothness"));
  dynamicsScoreItem->setSelectable(true);
  dynamicsScoreItem->setSelected(
      sequencerConfiguration()->dynamicsScoreEnabled());

  const QList<MenuItem *> menu{
      item,
      noteInfoItem,
      tempoVizItem,
      timingFeedbackItem,
      persistentMarksItem,
      handSyncScoreItem,
      dynamicsScoreItem,
      makeReverbSubmenu(synthesisConfiguration()->reverbPreset())};
  return makeMenu(
      muse::TranslatableString("appshell/menu/advanced", "A&dvanced"), menu,
      "menu-orchestrion-advanced");
}

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeReverbSubmenu(ReverbPreset current)
{
  using namespace muse::uicomponents;
  QList<MenuItem *> items;
  const auto addItem =
      [this, &items, current](const char *actionCode,
                              const muse::TranslatableString &title,
                              ReverbPreset preset)
  {
    auto *const item = makeMenuItem(actionCode, title);
    IF_ASSERT_FAILED(item) return;
    item->setSelectable(true);
    item->setSelected(preset == current);
    items.append(item);
  };

  addItem(actionIds::reverbOff,
          muse::TranslatableString("appshell/menu/advanced", "Off"),
          ReverbPreset::Off);
  addItem(actionIds::reverbRoom,
          muse::TranslatableString("appshell/menu/advanced", "Room"),
          ReverbPreset::Room);
  addItem(actionIds::reverbHall,
          muse::TranslatableString("appshell/menu/advanced", "Hall"),
          ReverbPreset::Hall);
  addItem(actionIds::reverbCathedral,
          muse::TranslatableString("appshell/menu/advanced", "Cathedral"),
          ReverbPreset::Cathedral);

  return makeMenu(muse::TranslatableString("appshell/menu/advanced", "&Reverb"),
                  items, "menu-orchestrion-reverb");
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