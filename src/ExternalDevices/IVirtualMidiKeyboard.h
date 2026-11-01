/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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

#include <async/notification.h>
#include <modularity/imoduleinterface.h>

#include <string>

namespace dgk
{
/**
 * A MIDI keyboard without the hardware: a MIDI input device among the others
 * (the Audio/MIDI menu lists it), played from the keyboard drawn on screen,
 * scripts or commands. Chosen, the app treats it as a plugged-in keyboard:
 * the keyboard icon shows it connected, its notes are played with their
 * velocity, its pedal sustains. Never chosen automatically.
 */
class IVirtualMidiKeyboard : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IVirtualMidiKeyboard);

public:
  virtual ~IVirtualMidiKeyboard() = default;

  /** Its id among the MIDI input devices. */
  virtual std::string deviceId() const = 0;

  /** Whether it is the MIDI input device in use: else it plays nothing. */
  virtual bool inUse() const = 0;
  virtual muse::async::Notification inUseChanged() const = 0;

  /** A key struck, `velocity` from 0 to 1, or 0 to release it, as in MIDI. */
  virtual void note(int pitch, float velocity) = 0;
  virtual void sustainPedal(bool down) = 0;
};
} // namespace dgk
