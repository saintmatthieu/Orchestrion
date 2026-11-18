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
#include "GestureInput.h"
#include "ComputerKeyboard/ComputerKeyboardGestureController.h"
#include "MidiDevice/MidiDeviceGestureController.h"
#include "OrchestrionCommon/LatencyTrace.h"
#include "OrchestrionCommon/OfflineRendering.h"

namespace dgk
{
GestureInput::~GestureInput() { deinit(); }

void GestureInput::deinit()
{
  // The producers first: they call Dispatch(), which uses the members
  // declared after them. The keyboard's controller lets go of the keyboard's
  // receiver, waiting for a key under way.
  m_keyboard.reset();
  // Where they were made: on the input thread, if there is one.
  const auto releaseMidiDevice = [this]
  {
    m_midiDeviceSubscriptions.reset();
    m_midiDevice.reset();
  };
  if (m_inputThread)
  {
    m_inputThread->Wait(releaseMidiDevice);
    m_inputThread.reset();
  }
  else
    releaseMidiDevice();
}

void GestureInput::init()
{
  m_keyboard = std::make_unique<ComputerKeyboardGestureController>(
      [this](const Gesture &gesture) { Dispatch(gesture); });

  m_midiDevice = std::make_unique<MidiDeviceGestureController>();
  const auto listenToMidiDevice = [this]
  {
    m_midiDeviceSubscriptions = std::make_unique<muse::async::Asyncable>();
    m_midiDevice->Listen();
    Listen(*m_midiDevice);
  };
  if (offlineRendering.audioPath.empty())
  {
    m_inputThread = std::make_unique<InputThread>();
    m_inputThread->Wait(listenToMidiDevice);
  }
  else
    listenToMidiDevice();
}

void GestureInput::Listen(IGestureController &controller)
{
  // Made where it lives
  const muse::async::Asyncable *const subscriber =
      m_midiDeviceSubscriptions.get();
  controller.noteOn().onReceive(
      subscriber, [this](int pitch, std::optional<float> velocity)
      { Dispatch({Gesture::Kind::noteOn, pitch, velocity, false}); });
  controller.noteOff().onReceive(
      subscriber, [this](int pitch)
      { Dispatch({Gesture::Kind::noteOff, pitch, std::nullopt, false}); });
  controller.sustainPedal().onReceive(
      subscriber, [this](bool down)
      { Dispatch({Gesture::Kind::sustainPedal, 0, std::nullopt, down}); });
}

void GestureInput::setReceiver(Receiver receiver)
{
  std::scoped_lock lock{m_receiverMutex};
  m_receiver = std::move(receiver);
}

void GestureInput::send(const Gesture &gesture) { Dispatch(gesture); }

void GestureInput::Dispatch(const Gesture &received)
{
  Gesture gesture = received;
  if (gesture.time == std::chrono::steady_clock::time_point{})
    gesture.time = std::chrono::steady_clock::now();
  if (gesture.kind != Gesture::Kind::sustainPedal)
    if (LatencyTrace *const trace = LatencyTrace::instance())
      gesture.traceId =
          trace->newNote(gesture.kind == Gesture::Kind::noteOn, gesture.pitch,
                         LatencyTrace::toNs(gesture.time));
  std::scoped_lock lock{m_receiverMutex};
  if (m_receiver)
    m_receiver(gesture);
}
} // namespace dgk
