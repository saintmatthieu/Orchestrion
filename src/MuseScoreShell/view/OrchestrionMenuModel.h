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

#include "uicomponents/view/abstractmenumodel.h"
#include <QWindow>

namespace dgk::orchestrion
{
class OrchestrionMenuModel : public muse::uicomponents::AbstractMenuModel
{
  Q_OBJECT

  Q_PROPERTY(QWindow *appWindow READ appWindow WRITE setAppWindow)
  Q_PROPERTY(QString openedMenuId READ openedMenuId WRITE setOpenedMenuId NOTIFY
                 openedMenuIdChanged)
  Q_PROPERTY(QRect appMenuAreaRect READ appMenuAreaRect WRITE setAppMenuAreaRect
                 NOTIFY appMenuAreaRectChanged)
  Q_PROPERTY(QRect openedMenuAreaRect READ openedMenuAreaRect WRITE
                 setOpenedMenuAreaRect NOTIFY openedMenuAreaRectChanged)

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

  muse::uicomponents::MenuItem *
  makeMenuItem(const muse::actions::ActionCode &actionCode,
               muse::uicomponents::MenuItemRole role);

  muse::uicomponents::MenuItem *makeFileMenu();

  QWindow *m_appWindow = nullptr;
  QRect m_appMenuAreaRect;
  QRect m_openedMenuAreaRect;
  QString m_openedMenuId;
};
} // namespace dgk::orchestrion
