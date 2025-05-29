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
#include "MidiDeviceGestureController.h"

namespace dgk
{
MidiDeviceGestureController::MidiDeviceGestureController()
{
  midiInPort()->eventReceived().onReceive(
      this, [this](const muse::midi::tick_t, const muse::midi::Event &event)
      { onMidiEventReceived(event); });
}

namespace
{
float compressVelocity(float velocity)
{
  // It's a little hard to play piano at a constant level. For now we hard-code
  // a compression. Making it parametric would be helpful, especially to use
  // harder compression for kids.
  constexpr auto minVelocity = 0.2f;
  return minVelocity + (1 - minVelocity) * velocity;
}
} // namespace

void MidiDeviceGestureController::onMidiEventReceived(
    const muse::midi::Event &event)
{
  if (event.isChannelVoice20())
  {
    auto events = event.toMIDI10();
    for (auto &midi10event : events)
      onMidiEventReceived(midi10event);
    return;
  }

  if (event.opcode() == muse::midi::Event::Opcode::NoteOn)
  {
    const auto velocity = event.velocity() / 128.0f;
    m_noteOn.send(event.note(), compressVelocity(velocity));
  }
  else if (event.opcode() == muse::midi::Event::Opcode::NoteOff)
    m_noteOff.send(event.note());
}

muse::async::Channel<int, float> MidiDeviceGestureController::noteOn() const
{
  return m_noteOn;
}

muse::async::Channel<int> MidiDeviceGestureController::noteOff() const
{
  return m_noteOff;
}
} // namespace dgk