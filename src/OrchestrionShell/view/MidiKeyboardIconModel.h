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
#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"

#include "async/asyncable.h"
#include "global/iglobalconfiguration.h"
#include "modularity/ioc.h"

#include <QObject>

namespace dgk
{
//! Backs the MIDI keyboard indicator in the score view's top-left corner:
//! whether a MIDI keyboard is connected (the icon dims and its hover panel
//! says so when not), and whether the icon is shown at all — hidden with its
//! cross, brought back from the View menu.
class MidiKeyboardIconModel : public QObject,
                              public muse::async::Asyncable,
                              public muse::Injectable
{
  Q_OBJECT

  Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
  Q_PROPERTY(bool iconVisible READ iconVisible NOTIFY iconVisibleChanged)
  Q_PROPERTY(QString iconSource READ iconSource CONSTANT)

  muse::Inject<IMidiDeviceService> midiDeviceService;
  muse::Inject<IOrchestrionSequencerConfiguration> sequencerConfiguration;
  muse::Inject<muse::IGlobalConfiguration> globalConfiguration;

public:
  explicit MidiKeyboardIconModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();
  //! The icon's cross: hides the icon until the View menu shows it again.
  Q_INVOKABLE void hide();

  bool connected() const;
  bool iconVisible() const;
  QString iconSource() const;

signals:
  void connectedChanged();
  void iconVisibleChanged();
};
} // namespace dgk
