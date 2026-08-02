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
#include "modularity/ioc.h"
#include <QObject>

namespace dgk
{
//! Connection status of physical controllers, backing the status icons shown
//! over the notation view. Today that is just the MIDI keyboard; a future
//! gamepad controller would get its own property here.
//! Also owns the global "T" shortcut that toggles the touchpad controller.
class ControllerStatusModel : public QObject,
                              public muse::async::Asyncable,
                              public muse::Injectable
{
  Q_OBJECT

  Q_PROPERTY(bool midiConnected READ midiConnected NOTIFY midiConnectedChanged)

  muse::Inject<IMidiDeviceService> midiDeviceService;
  muse::Inject<IGestureControllerSelector> gestureControllerSelector;

public:
  explicit ControllerStatusModel(QObject *parent = nullptr);

  Q_INVOKABLE void init();

  bool midiConnected() const;

signals:
  void midiConnectedChanged();

private:
  bool eventFilter(QObject *watched, QEvent *event) override;
};
} // namespace dgk
