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
#include "OrchestrionCommon/OfflineRendering.h"

namespace dgk
{
GestureInput::~GestureInput()
{
  if (m_inputThread)
    m_inputThread->Wait(
        [this]
        {
          m_midiDeviceSubscriptions.reset();
          m_midiDevice.reset();
        });
}

void GestureInput::init()
{
  m_keyboard = std::make_unique<ComputerKeyboardGestureController>();
  Listen(*m_keyboard);

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
  // The keyboard's subscriptions are this object's, the MIDI device's those
  // made where it lives.
  const muse::async::Asyncable *const subscriber =
      &controller == m_midiDevice.get() ? m_midiDeviceSubscriptions.get()
                                        : this;
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

void GestureInput::Dispatch(const Gesture &gesture)
{
  {
    std::scoped_lock lock{m_receiverMutex};
    if (m_receiver)
      m_receiver(gesture);
  }
  switch (gesture.kind)
  {
  case Gesture::Kind::noteOn:
    m_noteOn.send(gesture.pitch, gesture.velocity);
    break;
  case Gesture::Kind::noteOff:
    m_noteOff.send(gesture.pitch);
    break;
  case Gesture::Kind::sustainPedal:
    m_sustainPedal.send(gesture.down);
    break;
  }
}

muse::async::Channel<int, std::optional<float>> GestureInput::noteOn() const
{
  return m_noteOn;
}

muse::async::Channel<int> GestureInput::noteOff() const { return m_noteOff; }

muse::async::Channel<bool> GestureInput::sustainPedal() const
{
  return m_sustainPedal;
}
} // namespace dgk
