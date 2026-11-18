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

#include "ComputerKeyboard/ComputerKeyboardGestureController.h"
#include "IGestureController.h"
#include "IGestureInput.h"
#include "InputThread.h"
#include "MidiDevice/MidiDeviceGestureController.h"

#include <async/asyncable.h>

#include <memory>
#include <mutex>
#include <vector>

namespace dgk
{
class GestureInput : public IGestureInput, public muse::async::Asyncable
{
public:
  ~GestureInput() override;

  //! Creates the controllers; needs the IoC container populated (module init).
  void init();
  /**
   * Lets go of the controllers and stops the input thread: before the
   * message queues it processes stop (the global module's deinit).
   */
  void deinit();

  void setReceiver(Receiver) override;
  void send(const Gesture &) override;

private:
  /** Merges the controller's gestures in, from the thread they come from. */
  void Listen(IGestureController &);
  /** Stamps the gesture and tells the receiver, on the caller's thread. */
  void Dispatch(const Gesture &);

  /**
   * Where the MIDI device is listened to; none rendering offline, where
   * time is virtual on the main thread only, and all of it happens there.
   */
  std::unique_ptr<InputThread> m_inputThread;
  /**
   * Its gestures come from the main thread, or the raw keyboard's (see
   * IComputerKeyboard).
   */
  std::unique_ptr<ComputerKeyboardGestureController> m_keyboard;
  /**
   * Listens on the input thread, if there is one, and is destroyed there.
   */
  std::unique_ptr<MidiDeviceGestureController> m_midiDevice;
  /** Its subscriptions to the MIDI device, made and let go of there too. */
  std::unique_ptr<muse::async::Asyncable> m_midiDeviceSubscriptions;

  std::mutex m_receiverMutex;
  Receiver m_receiver;
};
} // namespace dgk
