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
#include "ControllerStatusModel.h"
#include <QApplication>
#include <QKeyEvent>

namespace dgk
{
ControllerStatusModel::ControllerStatusModel(QObject *parent) : QObject(parent)
{
  qApp->installEventFilter(this);
}

void ControllerStatusModel::init()
{
  midiDeviceService()->availableDevicesChanged().onNotify(
      this, [this] { emit midiConnectedChanged(); });
  midiDeviceService()->selectedDeviceChanged().onNotify(
      this, [this] { emit midiConnectedChanged(); });

  // The QML binding evaluates before this subscription is live, and the
  // service reshuffles the device selection during startup. Re-notify so the
  // binding cannot keep a value from mid-startup.
  emit midiConnectedChanged();
}

bool ControllerStatusModel::midiConnected() const
{
  return midiDeviceService()->realDeviceConnected();
}

bool ControllerStatusModel::eventFilter(QObject *watched, QEvent *event)
{
  if (event->type() == QEvent::KeyPress &&
      static_cast<QKeyEvent *>(event)->key() == Qt::Key_T &&
      gestureControllerSelector()->functionalControllers().count(
          GestureControllerType::Touchpad))
  {
    auto types = gestureControllerSelector()->selectedControllers();
    if (types.count(GestureControllerType::Touchpad))
      types.erase(GestureControllerType::Touchpad);
    else
      types.insert(GestureControllerType::Touchpad);
    gestureControllerSelector()->setSelectedControllers(types);
    return true;
  }
  return QObject::eventFilter(watched, event);
}
} // namespace dgk
