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
#pragma once

#include "ExternalDevices/IMidiDeviceService.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "OrchestrionShell/IOrchestrionUiActions.h"
#include "OrchestrionSynthesis/IEffectChain.h"
#include "OrchestrionSynthesis/IOrchestrionSynthesisConfiguration.h"
#include "muse_framework_config.h" // MUSE_APP_UNSTABLE
#include <QWindow>
#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <global/iglobalconfiguration.h>
#include <project/irecentfilescontroller.h>
#include <ui/iuiconfiguration.h>
#include <uicomponents/qml/Muse/UiComponents/abstractmenumodel.h>

namespace dgk
{
class OrchestrionMenuModel : public muse::uicomponents::AbstractMenuModel,
                             public muse::actions::Actionable
{
  Q_OBJECT

  Q_PROPERTY(QWindow *appWindow READ appWindow WRITE setAppWindow)
  Q_PROPERTY(QString openedMenuId READ openedMenuId WRITE setOpenedMenuId NOTIFY
                 openedMenuIdChanged)
  Q_PROPERTY(QRect appMenuAreaRect READ appMenuAreaRect WRITE setAppMenuAreaRect
                 NOTIFY appMenuAreaRectChanged)
  Q_PROPERTY(QRect openedMenuAreaRect READ openedMenuAreaRect WRITE
                 setOpenedMenuAreaRect NOTIFY openedMenuAreaRectChanged)

  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<muse::IGlobalConfiguration> globalConfiguration{this};
  dgk::Inject<muse::ui::IUiConfiguration> uiConfiguration{this};
  dgk::Inject<IOrchestrionUiActions> orchestrionUiActions{this};
  dgk::Inject<IMidiDeviceService> midiDeviceService{this};
  dgk::Inject<IOrchestrionSequencerConfiguration> sequencerConfiguration{this};
  dgk::Inject<IEffectChain> effectChain{this};
  dgk::Inject<IOrchestrionSynthesisConfiguration> synthesisConfiguration{this};
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<mu::project::IRecentFilesController> recentFilesController{this};

public:
  explicit OrchestrionMenuModel(QObject *parent = nullptr);

  QRect appMenuAreaRect() const;
  QRect openedMenuAreaRect() const;

  QWindow *appWindow() const;
  QString openedMenuId() const;

  Q_INVOKABLE void load() override;
  // Whether the platform has a global (out-of-window) menu bar, i.e. the
  // macOS top ribbon. Queried by MuseScore's PlatformMenuBar.qml.
  Q_INVOKABLE bool isGlobalMenuAvailable() const;
  Q_INVOKABLE void openMenu(const QString &menuId, bool byHover);

public slots:
  void setAppWindow(QWindow *appWindow);
  void setOpenedMenuId(QString openedMenuId);
  void setAppMenuAreaRect(QRect appMenuAreaRect);
  void setOpenedMenuAreaRect(QRect openedMenuAreaRect);

signals:
  void openMenuRequested(const QString &menuId, bool byHover);
  void closeOpenedMenuRequested();
  void openedMenuIdChanged(QString openedMenuId);
  void appMenuAreaRectChanged(QRect appMenuAreaRect);
  void openedMenuAreaRectChanged(QRect openedMenuAreaRect);

private:
  using muse::uicomponents::AbstractMenuModel::makeMenuItem;

  muse::uicomponents::MenuItem *makeFileMenu(bool velocityRecordingEnabled);
  muse::uicomponents::MenuItem *makeExampleScoresSubmenu();
  muse::uicomponents::MenuItem *makeRecentScoresSubmenu();
  QList<muse::uicomponents::MenuItem *> makeRecentScoresItems();
  /**
   * Refreshes the "Open recent" submenu in place, from the current
   * recent-files list.
   */
  void updateRecentScoresSubmenu();
  /**
   * An item that opens the score at `url`, titled `title`;
   * `displayNameOverride` is forwarded to the open action (see
   * mu::project::ProjectFile::displayNameOverride).
   */
  muse::uicomponents::MenuItem *
  makeOpenScoreItem(const QString &id, const QUrl &url, const QString &title,
                    const QString &displayNameOverride);
  muse::uicomponents::MenuItem *makeViewMenu();
  muse::uicomponents::MenuItem *makeHelpMenu();
  muse::uicomponents::MenuItem *makeAudioMidiMenu();
  muse::uicomponents::MenuItem *makeAdvancedMenu(bool velocityRecordingEnabled);
  muse::uicomponents::MenuItem *makeGradingMenu();
  muse::uicomponents::MenuItem *makeAutoPlayMenu();
  muse::uicomponents::MenuItem *makeOrnamentsMenu();
#ifdef MUSE_APP_UNSTABLE
  muse::uicomponents::MenuItem *makeDevelopmentMenu();
#endif
  muse::uicomponents::MenuItem *makeAudioMidiSubmenu(DeviceType);
  muse::uicomponents::MenuItem *makeReverbSubmenu(ReverbPreset current);
  muse::uicomponents::MenuItem *makeEffectsMenu();
  QList<muse::uicomponents::MenuItem *> makeEffectsMenuItems();
  void updateEffectsMenu();

  void createMenus(bool withSaveItem);

  QList<muse::uicomponents::MenuItem *>
  getMenuItems(const std::vector<DeviceAction> &devices);

  void updateMenuItems(const std::vector<DeviceAction> &devices,
                       const std::string &menuId);
  void selectMenuItem(const char *submenuId, const std::string &deviceId);

  QWindow *m_appWindow = nullptr;
  QRect m_appMenuAreaRect;
  QRect m_openedMenuAreaRect;
  QString m_openedMenuId;
};
} // namespace dgk
