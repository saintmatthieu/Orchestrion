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
#include "OrchestrionCommon/OrchestrionPalette.h"
#include "log.h"
#include "types/translatablestring.h"

#include <QDir>
#include <map>

namespace dgk
{
namespace
{
constexpr auto audioMidiMenuId = "menu-audio-midi";
constexpr auto effectsMenuId = "menu-orchestrion-effects";
constexpr auto keyboardMenuId = "menu-keyboard";
constexpr auto themeMenuId = "menu-orchestrion-theme";
constexpr auto recentScoresMenuId = "menu-orchestrion-recent-scores";
constexpr auto toggleRecordingMenuId = "orchestrion-advanced-toggle-recording";
constexpr auto toggleNoteInfoMenuId = "orchestrion-advanced-toggle-note-info";
constexpr auto toggleTempoVizMenuId =
    "orchestrion-advanced-toggle-tempo-visualization";
constexpr auto toggleGradingMenuId = "orchestrion-advanced-toggle-grading";
constexpr auto togglePersistentTimingMarksMenuId =
    "orchestrion-advanced-toggle-persistent-timing-marks";
constexpr auto toggleHandSyncScoreMenuId =
    "orchestrion-advanced-toggle-hand-sync-score";
constexpr auto toggleDynamicsScoreMenuId =
    "orchestrion-advanced-toggle-dynamics-score";
constexpr auto toggleProportionalSpacingMenuId =
    "orchestrion-advanced-toggle-proportional-spacing";
} // namespace

OrchestrionMenuModel::OrchestrionMenuModel(QObject *parent)
    : AbstractMenuModel(parent)
{
}

QWindow *OrchestrionMenuModel::appWindow() const { return m_appWindow; }

bool OrchestrionMenuModel::isGlobalMenuAvailable() const
{
  return uiConfiguration()->isGlobalMenuAvailable();
}

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
  QList<muse::uicomponents::MenuItem *> menus{
      makeFileMenu(velocityRecordingEnabled), makeViewMenu(),
      makeAudioMidiMenu(), makeEffectsMenu(), makeOrnamentsMenu()};
  if (sequencerConfiguration()->gradingExposed())
    menus << makeGradingMenu();
  if (sequencerConfiguration()->autoPlayExposed())
    menus << makeAutoPlayMenu();
  menus << makeAdvancedMenu(velocityRecordingEnabled);
#ifdef MUSE_APP_UNSTABLE
  menus << makeDevelopmentMenu();
#endif
  menus << makeHelpMenu();
  setItems(menus);
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

  sequencerConfiguration()->midiKeyboardIconVisibleChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->pedalIndicatorVisibleChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->ornamentModeChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->noteInfoTooltipEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->tempoVisualizationEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->jumpAnticipationEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->gradingEnabledChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->autoPlayedStaffChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->gradingExposedChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  sequencerConfiguration()->autoPlayExposedChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  uiConfiguration()->currentThemeChanged().onNotify(
      this, [this]
      { createMenus(sequencerConfiguration()->velocityRecordingEnabled()); });

  recentFilesController()->recentFilesListChanged().onNotify(
      this, [this] { updateRecentScoresSubmenu(); });

  effectChain()->availableEffectsChanged().onNotify(this, [this]
                                                    { updateEffectsMenu(); });
  effectChain()->chainChanged().onNotify(this, [this] { updateEffectsMenu(); });
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
                [](MenuItem *item) { item->setChecked(false); });
  const auto it = std::find_if(
      subitems.begin(), subitems.end(), [deviceId](const MenuItem *item)
      { return item->args().arg<std::string>(1) == deviceId; });
  if (it == subitems.end())
    return;
  (*it)->setChecked(true);
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
  {
    for (auto deviceType : kDeviceTypes)
    {
      const auto menuId = actionIds::chooseDevicesSubmenu.at(deviceType);
      updateMenuItems(orchestrionUiActions()->settableDevices(deviceType),
                      menuId);
      selectMenuItem(menuId,
                     orchestrionUiActions()->selectedDevice(deviceType));
    }
  }
  else if (menuId == effectsMenuId)
    updateEffectsMenu();
  emit openMenuRequested(menuId, byHover);
}

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeFileMenu(bool withSaveItem)
{
  QList<muse::uicomponents::MenuItem *> menu{
      makeMenuItem("orchestrion-file-open",
                   muse::TranslatableString("appshell/menu/file",
                                            "Open from &computer…")),
      makeRecentScoresSubmenu(),
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
  using namespace muse::uicomponents;

  // Whether the auto-scroll moves on to a jump's resume point ahead of time
  // (see IOrchestrionSequencerConfiguration::jumpAnticipationEnabled).
  MenuItem *const jumpAnticipationItem = makeMenuItem(
      actionIds::toggleJumpAnticipation,
      muse::TranslatableString("appshell/menu/view", "&Anticipate jumps"));
  jumpAnticipationItem->setCheckable(true);
  jumpAnticipationItem->setChecked(
      sequencerConfiguration()->jumpAnticipationEnabled());

  // The MIDI keyboard indicator can be dismissed with its own cross; this is
  // where it comes back.
  MenuItem *const midiIconItem = makeMenuItem(
      actionIds::toggleMidiKeyboardIcon,
      muse::TranslatableString("appshell/menu/view", "&MIDI keyboard icon"));
  midiIconItem->setCheckable(true);
  midiIconItem->setChecked(sequencerConfiguration()->midiKeyboardIconVisible());

  // The sustain-pedal indicator at the bottom of the score view.
  MenuItem *const pedalItem = makeMenuItem(
      actionIds::togglePedalIndicator,
      muse::TranslatableString("appshell/menu/view", "&Pedal indicator"));
  pedalItem->setCheckable(true);
  pedalItem->setChecked(sequencerConfiguration()->pedalIndicatorVisible());

  QList<muse::uicomponents::MenuItem *> menu{
      jumpAnticipationItem, midiIconItem, pedalItem, makeThemeSubmenu(),
      makeMenuItem(
          "view-toggle-fullscreen",
          muse::TranslatableString("appshell/menu/view", "&Fullscreen"))};

  return makeMenu(muse::TranslatableString("appshell/menu/view", "&View"), menu,
                  "menu-orchestrion-view");
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeThemeSubmenu()
{
  using namespace muse::uicomponents;

  // The two looks Orchestrion ships. They are radio items rather than a
  // single toggle: the check has to say which one is on, and a third theme
  // would drop in beside them.
  const auto current =
      paletteString(uiConfiguration()->currentTheme(), paletteKeys::themeName);

  const auto makeThemeItem = [&](const char *actionId,
                                 const muse::TranslatableString &title,
                                 const char *themeName)
  {
    MenuItem *const item = makeMenuItem(actionId, title);
    item->setCheckable(true);
    item->setChecked(current == QString::fromLatin1(themeName));
    return item;
  };

  QList<MenuItem *> items{
      makeThemeItem(actionIds::setGoldTheme,
                    muse::TranslatableString("appshell/menu/view", "&Gold"),
                    goldThemeName),
      makeThemeItem(actionIds::setSilverTheme,
                    muse::TranslatableString("appshell/menu/view", "&Silver"),
                    silverThemeName)};

  return makeMenu(muse::TranslatableString("appshell/menu/view", "&Theme"),
                  items, themeMenuId);
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
    items.append(
        makeOpenScoreItem(QString("example-score-%1").arg(index++),
                          QUrl::fromLocalFile(entry.absoluteFilePath()),
                          displayName, displayName));
  }

  return makeMenu(
      muse::TranslatableString("appshell/menu/file", "&Example scores"), items,
      "menu-orchestrion-example-scores");
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeRecentScoresSubmenu()
{
  const QList<muse::uicomponents::MenuItem *> items = makeRecentScoresItems();
  return makeMenu(
      muse::TranslatableString("appshell/menu/file", "Open &recent"), items,
      recentScoresMenuId, /*enabled=*/!items.empty());
}

QList<muse::uicomponents::MenuItem *>
OrchestrionMenuModel::makeRecentScoresItems()
{
  using namespace muse::uicomponents;

  // MuseScore's project module maintains the list (on open and on save);
  // this only reads it. Scores are named as in the "Example scores" submenu:
  // file name without extension, underscores as spaces.
  QList<MenuItem *> items;
  int index = 0;
  for (const mu::project::RecentFile &file :
       recentFilesController()->recentFilesList())
    items.append(makeOpenScoreItem(
        QString("recent-score-%1").arg(index++), file.path.toQUrl(),
        file.displayName(/*includingExtension=*/false).replace('_', ' '),
        file.displayNameOverride));

  if (!items.empty())
  {
    items.append(makeSeparator());
    items.append(makeMenuItem(
        actionIds::clearRecentFiles,
        muse::TranslatableString("appshell/menu/file", "&Clear recent files")));
  }

  return items;
}

void OrchestrionMenuModel::updateRecentScoresSubmenu()
{
  using namespace muse::uicomponents;
  MenuItem &menu = findItem(QString{recentScoresMenuId});
  IF_ASSERT_FAILED(menu.isValid()) return;
  const QList<MenuItem *> items = makeRecentScoresItems();
  menu.setSubitems(items);
  menu.setEnabled(!items.empty());
  emit itemChanged(&menu);
}

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeOpenScoreItem(const QString &id, const QUrl &url,
                                        const QString &title,
                                        const QString &displayNameOverride)
{
  using namespace muse::uicomponents;

  auto *item = new MenuItem(this);
  item->setId(id);

  muse::ui::UiAction action;
  action.code = "orchestrion-file-open";
  action.title = muse::TranslatableString::untranslatable(
      muse::String::fromQString(title));
  item->setAction(action);

  muse::ui::UiActionState state;
  state.enabled = true;
  item->setState(state);

  item->setArgs(muse::actions::ActionData::make_arg2<QUrl, QString>(
      url, displayNameOverride));

  return item;
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

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeEffectsMenu()
{
  return makeMenu(muse::TranslatableString("appshell/menu/effects", "&Effects"),
                  makeEffectsMenuItems(), effectsMenuId);
}

QList<muse::uicomponents::MenuItem *>
OrchestrionMenuModel::makeEffectsMenuItems()
{
  using namespace muse::uicomponents;
  // The master effect chain: the effects in it, in processing order, each
  // with its editor and its removal, then what can be added. These aren't
  // registered UI actions, one per effect: like the example scores, they are
  // items of a shared action taking the effect's id as argument.
  const std::vector<EffectDesc> chain = effectChain()->chain();
  std::vector<EffectDesc> effects = effectChain()->availableEffects();
  // Orchestrion's own effects sit at the top of the Add list; the plugins
  // below them.
  std::vector<EffectDesc> builtIn;
  std::copy_if(effects.begin(), effects.end(), std::back_inserter(builtIn),
               [](const EffectDesc &effect) { return effect.builtIn; });
  effects.erase(std::remove_if(effects.begin(), effects.end(),
                               [](const EffectDesc &effect)
                               { return effect.builtIn; }),
                effects.end());
  int index = 0;
  const auto makeItem = [this, &index](const char *actionCode,
                                       const muse::TranslatableString &title,
                                       const std::string &effectId)
  {
    auto *item = new MenuItem(this);
    item->setId(QString("effect-%1").arg(index++));
    muse::ui::UiAction action;
    action.code = actionCode;
    action.title = title;
    item->setAction(action);
    muse::ui::UiActionState state;
    state.enabled = !effectId.empty();
    item->setState(state);
    if (!effectId.empty())
      item->setArgs(
          muse::actions::ActionData::make_arg1<std::string>(effectId));
    return item;
  };
  const auto untranslatable = [](const std::string &text)
  {
    return muse::TranslatableString::untranslatable(
        muse::String::fromStdString(text));
  };

  QList<MenuItem *> items;
  for (const EffectDesc &entry : chain)
  {
    // An effect whose plugin has gone missing must remain removable.
    const bool available =
        entry.builtIn || std::any_of(effects.begin(), effects.end(),
                                     [&entry](const EffectDesc &effect)
                                     { return effect.id == entry.id; });
    const QList<MenuItem *> entryItems{
        makeItem(actionIds::editEffect,
                 muse::TranslatableString("appshell/menu/effects", "&Show…"),
                 entry.id),
        makeItem(actionIds::removeEffect,
                 muse::TranslatableString("appshell/menu/effects", "&Remove"),
                 entry.id)};
    const muse::String name = muse::String::fromStdString(entry.name);
    items.append(makeMenu(
        !available ? muse::TranslatableString("appshell/menu/effects",
                                              "%1 (not found)")
                         .arg(name)
        : !entry.active
            ? muse::TranslatableString("appshell/menu/effects", "%1 (bypassed)")
                  .arg(name)
            : untranslatable(entry.name),
        entryItems, QString("effect-chain-%1").arg(index++)));
  }
  if (!chain.empty())
    items.append(makeSeparator());

  // What can be added. An effect already in the chain is shown checked and
  // greyed out. A menu taller than the screen does not open at all, so a long
  // list is split by vendor.
  const auto makeAddItem = [&](const EffectDesc &effect)
  {
    MenuItem *const item =
        makeItem(actionIds::addEffect, untranslatable(effect.name), effect.id);
    if (effectChain()->contains(effect.id))
    {
      item->setCheckable(true);
      muse::ui::UiActionState state;
      state.enabled = false;
      state.checked = true;
      item->setState(state);
    }
    return item;
  };
  constexpr size_t maxFlatListSize = 12;
  QList<MenuItem *> addItems;
  for (const EffectDesc &effect : builtIn)
    addItems.append(makeAddItem(effect));
  if (!builtIn.empty())
    addItems.append(makeSeparator());
  if (effects.empty())
    addItems.append(makeItem(actionIds::addEffect,
                             muse::TranslatableString("appshell/menu/effects",
                                                      "No VST3 effects found"),
                             {}));
  else if (effects.size() <= maxFlatListSize)
    for (const EffectDesc &effect : effects)
      addItems.append(makeAddItem(effect));
  else
  {
    std::map<std::string, std::vector<EffectDesc>> byVendor;
    for (const EffectDesc &effect : effects)
      byVendor[effect.vendor].push_back(effect);
    for (const auto &[vendor, vendorEffects] : byVendor)
    {
      QList<MenuItem *> vendorItems;
      for (const EffectDesc &effect : vendorEffects)
        vendorItems.append(makeAddItem(effect));
      addItems.append(makeMenu(
          vendor.empty() ? muse::TranslatableString("appshell/menu/effects",
                                                    "Unknown vendor")
                         : untranslatable(vendor),
          vendorItems, QString("effect-vendor-%1").arg(index++)));
    }
  }
  items.append(
      makeMenu(muse::TranslatableString("appshell/menu/effects", "&Add"),
               addItems, "effect-add"));
  return items;
}

void OrchestrionMenuModel::updateEffectsMenu()
{
  using namespace muse::uicomponents;
  MenuItem &menu = findItem(QString{effectsMenuId});
  if (!menu.isValid())
    return;
  menu.setSubitems(makeEffectsMenuItems());
  emit itemChanged(&menu);
}

muse::uicomponents::MenuItem *
OrchestrionMenuModel::makeAdvancedMenu(bool velocityRecordingEnabled)
{
  using namespace muse::uicomponents;
  muse::uicomponents::MenuItem *const item =
      makeMenuItem(toggleRecordingMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "&Toggle velocity recording"));
  item->setCheckable(true);
  item->setChecked(velocityRecordingEnabled);

  muse::uicomponents::MenuItem *const noteInfoItem =
      makeMenuItem(toggleNoteInfoMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "Show &note info on hover"));
  noteInfoItem->setCheckable(true);
  noteInfoItem->setChecked(sequencerConfiguration()->noteInfoTooltipEnabled());

  muse::uicomponents::MenuItem *const tempoVizItem =
      makeMenuItem(toggleTempoVizMenuId,
                   muse::TranslatableString("appshell/menu/advanced",
                                            "Show &tempo visualization"));
  tempoVizItem->setCheckable(true);
  tempoVizItem->setChecked(
      sequencerConfiguration()->tempoVisualizationEnabled());

  const QList<MenuItem *> menu{item, noteInfoItem, tempoVizItem};
  return makeMenu(
      muse::TranslatableString("appshell/menu/advanced", "A&dvanced"), menu,
      "menu-orchestrion-advanced");
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeAutoPlayMenu()
{
  using namespace muse::uicomponents;

  // Which hand the machine plays for you: a one-of-three choice, mirrored by
  // the top-row button's popup.
  const int autoPlayedStaff = sequencerConfiguration()->autoPlayedStaff();
  const auto choice = [this](const char *actionCode,
                             const muse::TranslatableString &title,
                             bool selected)
  {
    MenuItem *const item = makeMenuItem(actionCode, title);
    item->setCheckable(true);
    item->setChecked(selected);
    return item;
  };

  return makeMenu(
      muse::TranslatableString("appshell/menu/autoplay", "&Auto-play"),
      QList<MenuItem *>{
          choice(actionIds::autoPlayNone,
                 muse::TranslatableString("appshell/menu/autoplay", "&Off"),
                 autoPlayedStaff < 0),
          choice(
              actionIds::autoPlayLeftHand,
              muse::TranslatableString("appshell/menu/autoplay", "&Left hand"),
              autoPlayedStaff == 1),
          choice(
              actionIds::autoPlayRightHand,
              muse::TranslatableString("appshell/menu/autoplay", "&Right hand"),
              autoPlayedStaff == 0)},
      "menu-orchestrion-autoplay");
}

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeOrnamentsMenu()
{
  using namespace muse::uicomponents;

  // How the score's ornaments are played: a one-of-three choice.
  const OrnamentMode mode = sequencerConfiguration()->ornamentMode();
  const auto choice = [this, mode](const char *actionCode,
                                   const muse::TranslatableString &title,
                                   OrnamentMode value)
  {
    MenuItem *const item = makeMenuItem(actionCode, title);
    item->setCheckable(true);
    item->setChecked(mode == value);
    return item;
  };

  return makeMenu(
      muse::TranslatableString("appshell/menu/ornaments", "&Ornaments"),
      QList<MenuItem *>{
          choice(
              actionIds::ornamentsDisabled,
              muse::TranslatableString("appshell/menu/ornaments", "&Disabled"),
              OrnamentMode::disabled),
          choice(actionIds::ornamentsManual,
                 muse::TranslatableString("appshell/menu/ornaments", "&Manual"),
                 OrnamentMode::manual),
          choice(
              actionIds::ornamentsAutomatic,
              muse::TranslatableString("appshell/menu/ornaments", "&Automatic"),
              OrnamentMode::automatic)},
      "menu-orchestrion-ornaments");
}

#ifdef MUSE_APP_UNSTABLE
muse::uicomponents::MenuItem *OrchestrionMenuModel::makeDevelopmentMenu()
{
  using namespace muse::uicomponents;

  // Auto-play is still being worked on: hidden — and inert — unless a
  // developer asks for it here.
  MenuItem *const autoPlayItem =
      makeMenuItem(actionIds::toggleAutoPlayExposure,
                   muse::TranslatableString("appshell/menu/development",
                                            "Expose &auto-play"));
  autoPlayItem->setCheckable(true);
  autoPlayItem->setChecked(sequencerConfiguration()->autoPlayExposed());

  MenuItem *const gradingItem = makeMenuItem(
      actionIds::toggleGradingExposure,
      muse::TranslatableString("appshell/menu/development", "Expose &grading"));
  gradingItem->setCheckable(true);
  gradingItem->setChecked(sequencerConfiguration()->gradingExposed());

  return makeMenu(
      muse::TranslatableString("appshell/menu/development", "&Development"),
      QList<MenuItem *>{gradingItem, autoPlayItem},
      "menu-orchestrion-development");
}
#endif

muse::uicomponents::MenuItem *OrchestrionMenuModel::makeGradingMenu()
{
  using namespace muse::uicomponents;

  // The master switch (also the top-row toggle button); everything it
  // governs is configured in the grading settings dialog.
  MenuItem *const toggleItem = makeMenuItem(
      toggleGradingMenuId,
      muse::TranslatableString("appshell/menu/grading", "&Enabled"));
  toggleItem->setCheckable(true);
  toggleItem->setChecked(sequencerConfiguration()->gradingEnabled());

  MenuItem *const settingsItem = makeMenuItem(
      actionIds::gradingSettings,
      muse::TranslatableString("appshell/menu/grading", "&Settings…"));

  return makeMenu(muse::TranslatableString("appshell/menu/grading", "&Grading"),
                  QList<MenuItem *>{toggleItem, settingsItem},
                  "menu-orchestrion-grading");
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
                  item->setCheckable(true);
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