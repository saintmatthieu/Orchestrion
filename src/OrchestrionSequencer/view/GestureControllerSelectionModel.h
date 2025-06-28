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
#include "GestureControllers/IGestureControllerSelector.h"

#include "async/asyncable.h"
#include "global/iglobalconfiguration.h"
#include "modularity/ioc.h"
#include <QAbstractListModel>

namespace dgk
{
class ControllerInfo
{
  Q_GADGET;
  Q_PROPERTY(bool isWorking MEMBER isWorking);
  Q_PROPERTY(QString icon MEMBER icon);

public:
  ControllerInfo() = default;
  ControllerInfo(bool isWorking, QString icon, GestureControllerType type);

  bool isWorking = false;
  QString icon;
  GestureControllerType type;
};

class GestureControllerSelectionModel : public QAbstractListModel,
                                        public muse::async::Asyncable,
                                        public muse::Injectable
{
  Q_OBJECT;

  Q_PROPERTY(QList<ControllerInfo> selectedControllersInfo READ
                 selectedControllersInfo NOTIFY selectedControllersInfoChanged);
  Q_PROPERTY(QString warningIcon READ warningIcon CONSTANT);
  Q_PROPERTY(bool hasWarning READ hasWarning NOTIFY selectedControllersInfoChanged);

  muse::Inject<IGestureControllerSelector> gestureControllerSelector;
  muse::Inject<IMidiDeviceService> midiDeviceService;
  muse::Inject<muse::IGlobalConfiguration> globalConfiguration;

public:
  explicit GestureControllerSelectionModel(QObject *parent = nullptr);

  Q_INVOKABLE void init();
  Q_INVOKABLE void updateControllerIsSelected(int index, bool checked);

  QList<ControllerInfo> selectedControllersInfo() const;
  QString warningIcon() const;
  bool hasWarning() const;

signals:
  void selectedControllersInfoChanged();
  void hasWarningChanged();

private:
  enum RoleNames
  {
    rControllerIsSelected = Qt::UserRole + 1,
    rControllerIsWorking = Qt::UserRole + 2,
    rControllerName = Qt::UserRole + 3,
  };

  std::vector<GestureControllerType> sortedControllers() const;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  bool eventFilter(QObject *watched, QEvent *event) override;

  QString itemName(int index) const;
  QString itemIcon(GestureControllerType) const;
  QString iconDir() const;
  bool isControllerWorking(int index) const;
  bool isControllerWorking(GestureControllerType) const;
  void emitSignals();

  std::vector<GestureControllerType> m_selectedControllerQueue;
};
} // namespace dgk
