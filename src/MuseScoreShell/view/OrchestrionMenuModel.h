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

#include "Orchestrion/IOrchestrionSequencerUiActions.h"
#include "OrchestrionShell/IControllerMenuManager.h"
#include "OrchestrionShell/IOrchestrionUiActions.h"
#include "OrchestrionShell/IPlaybackDeviceManager.h"
#include "OrchestrionShell/ISynthesizerMenuManager.h"
#include "Orchestrion/IComputerKeyboard.h"
#include <QWindow>
#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <uicomponents/view/abstractmenumodel.h>

namespace dgk::orchestrion
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

  muse::Inject<muse::actions::IActionsDispatcher> dispatcher = {this};
  muse::Inject<IOrchestrionUiActions> orchestrionUiActions = {this};
  muse::Inject<IControllerMenuManager> midiControllerManager = {this};
  muse::Inject<ISynthesizerMenuManager> midiSynthesizerManager = {this};
  muse::Inject<IPlaybackDeviceManager> playbackDeviceManager = {this};
  muse::Inject<dgk::IComputerKeyboard> computerKeyboard = {this};
  muse::Inject<IOrchestrionSequencerUiActions> uiActions = {this};

public:
  explicit OrchestrionMenuModel(QObject *parent = nullptr);

  QRect appMenuAreaRect() const;
  QRect openedMenuAreaRect() const;

  QWindow *appWindow() const;
  QString openedMenuId() const;

  Q_INVOKABLE void load() override;
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

  muse::uicomponents::MenuItem *makeFileMenu();
  muse::uicomponents::MenuItem *makeAudioMidiMenu();
  muse::uicomponents::MenuItem *makeAudioMidiSubmenu(DeviceType);
  muse::uicomponents::MenuItem *makeKeyboardMenu();

  QList<muse::uicomponents::MenuItem *>
  getMenuItems(const std::vector<DeviceAction> &devices);

  void updateMenuItems(const std::vector<DeviceAction> &devices,
                       const std::string &menuId);
  void selectMenuItem(const char *submenuId, const std::string &deviceId);
  void updateSelectedKeyboardMenuItem();

  QWindow *m_appWindow = nullptr;
  QRect m_appMenuAreaRect;
  QRect m_openedMenuAreaRect;
  QString m_openedMenuId;
};
} // namespace dgk::orchestrion
