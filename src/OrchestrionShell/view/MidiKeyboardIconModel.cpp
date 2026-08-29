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
#include "MidiKeyboardIconModel.h"

namespace dgk
{
MidiKeyboardIconModel::MidiKeyboardIconModel(QObject *parent) : QObject(parent)
{
}

void MidiKeyboardIconModel::load()
{
  midiDeviceService()->availableDevicesChanged().onNotify(
      this, [this] { emit connectedChanged(); });
  midiDeviceService()->selectedDeviceChanged().onNotify(
      this, [this] { emit connectedChanged(); });
  sequencerConfiguration()->midiKeyboardIconVisibleChanged().onNotify(
      this, [this] { emit iconVisibleChanged(); });

  // The view's bindings were evaluated before we subscribed, and the MIDI
  // device's startup selection may have happened in between.
  emit connectedChanged();
  emit iconVisibleChanged();
}

void MidiKeyboardIconModel::hide()
{
  sequencerConfiguration()->setMidiKeyboardIconVisible(false);
}

bool MidiKeyboardIconModel::connected() const
{
  const auto device = midiDeviceService()->selectedDevice();
  return device && midiDeviceService()->isAvailable(*device) &&
         !midiDeviceService()->isNoDevice(*device);
}

bool MidiKeyboardIconModel::iconVisible() const
{
  return sequencerConfiguration()->midiKeyboardIconVisible();
}

QString MidiKeyboardIconModel::iconSource() const
{
  return QString{"file:///"} +
         globalConfiguration()->appDataPath().toQString() +
         "icons/controllers/piano_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg";
}
} // namespace dgk
