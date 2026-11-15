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

#include "IGestureController.h"

#include <functional>
#include <modularity/imoduleinterface.h>
#include <optional>

namespace dgk
{
//! The note events of every gesture controller, merged into one stream: the
//! computer keyboard, always, and the MIDI device chosen in the Audio/MIDI
//! menu, whenever one is connected. There is nothing to select — whatever can
//! produce notes does. The same goes for the sustain pedal plugged into the
//! MIDI keyboard, if any.
//!
//! The MIDI device's come from a thread of its own (see InputThread), the
//! computer keyboard's from the main thread.
class IGestureInput : public IGestureController, MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IGestureInput);

public:
  virtual ~IGestureInput() = default;

  /** A gesture, as the channels tell it. */
  struct Gesture
  {
    enum class Kind
    {
      noteOn,
      noteOff,
      sustainPedal,
    };
    Kind kind = Kind::noteOn;
    int pitch = 0;
    /** A note-on's, if the controller measures it. */
    std::optional<float> velocity;
    /** A sustain pedal's. */
    bool down = false;
  };
  using Receiver = std::function<void(const Gesture &)>;

  /**
   * Told of every gesture, before the channels are, on the thread it comes
   * from, which needn't be the main thread: for the sequencer, which takes
   * them on a thread of its own. Must be quick, and safe from any thread.
   */
  virtual void setReceiver(Receiver) = 0;

  /**
   * A gesture from elsewhere (a take's replay), as if a controller had made
   * it. From the main thread.
   */
  virtual void send(const Gesture &) = 0;
};
} // namespace dgk
