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
#include "VirtualKeyboardModel.h"

namespace dgk
{
namespace
{
constexpr int pitchCount = 128;
// A piano's keys, A0 to C8
constexpr int lowestKey = 21;
constexpr int highestKey = 108;

const char *RoleName(IOrchestrionSequencer::KeyRole role)
{
  switch (role)
  {
  case IOrchestrionSequencer::KeyRole::leftHand:
    return "left";
  case IOrchestrionSequencer::KeyRole::rightHand:
    return "right";
  case IOrchestrionSequencer::KeyRole::notes:
    return "notes";
  case IOrchestrionSequencer::KeyRole::none:
    break;
  }
  return "";
}
constexpr int sustainPedalController = 64;
} // namespace

VirtualKeyboardModel::VirtualKeyboardModel(QObject *parent) : QObject(parent)
{
  for (int i = 0; i < pitchCount; ++i)
    m_velocities.push_back(0.0);
}

void VirtualKeyboardModel::load()
{
  virtualKeyboard()->inUseChanged().onNotify(this,
                                             [this] { emit shownChanged(); });
  tutorials()->runningChanged().onNotify(this, [this] { emit shownChanged(); });
  midiInPort()->eventReceived().onReceive(
      this, [this](muse::midi::tick_t, const muse::midi::Event &event)
      { onEvent(event); });
  // Another score, other modes: other ranges
  orchestrion()->sequencerChanged().onNotify(this, [this] { updateRanges(); });
  staffModes()->modesChanged().onNotify(this, [this] { updateRanges(); });
  updateRanges();
  emit shownChanged();
}

void VirtualKeyboardModel::press(int pitch, double velocity)
{
  virtualKeyboard()->note(pitch, static_cast<float>(velocity));
}

void VirtualKeyboardModel::release(int pitch)
{
  virtualKeyboard()->note(pitch, 0);
}

bool VirtualKeyboardModel::shown() const
{
  return virtualKeyboard()->inUse() && !tutorials()->hidesVirtualKeyboard();
}

QVariantList VirtualKeyboardModel::velocities() const { return m_velocities; }

bool VirtualKeyboardModel::pedalDown() const { return m_pedalDown; }

QVariantList VirtualKeyboardModel::ranges() const { return m_ranges; }

void VirtualKeyboardModel::updateRanges()
{
  QVariantList ranges;
  if (const auto sequencer = orchestrion()->sequencer())
  {
    auto role = IOrchestrionSequencer::KeyRole::none;
    int from = lowestKey;
    for (int pitch = lowestKey; pitch <= highestKey + 1; ++pitch)
    {
      const auto next = pitch <= highestKey
                            ? sequencer->RoleOfKey(pitch)
                            : IOrchestrionSequencer::KeyRole::none;
      if (pitch > lowestKey && next == role)
        continue;
      if (pitch > lowestKey && role != IOrchestrionSequencer::KeyRole::none)
        ranges.push_back(QVariantMap{
            {"from", from}, {"to", pitch - 1}, {"role", RoleName(role)}});
      role = next;
      from = pitch;
    }
  }
  if (ranges == m_ranges)
    return;
  m_ranges = ranges;
  emit rangesChanged();
}

void VirtualKeyboardModel::onEvent(const muse::midi::Event &event)
{
  if (event.isChannelVoice20())
  {
    for (const auto &midi10 : event.toMIDI10())
      onEvent(midi10);
    return;
  }
  using Opcode = muse::midi::Event::Opcode;
  if (event.opcode() == Opcode::NoteOn)
    // A note-on at velocity 0 is a release, as MIDI has it
    setVelocity(event.note(), event.velocity7() / 127.0);
  else if (event.opcode() == Opcode::NoteOff)
    setVelocity(event.note(), 0.0);
  else if (event.opcode() == Opcode::ControlChange &&
           event.index() == sustainPedalController)
  {
    const bool down = event.data7() >= 64;
    if (down == m_pedalDown)
      return;
    m_pedalDown = down;
    emit pedalDownChanged();
  }
}

void VirtualKeyboardModel::setVelocity(int pitch, double velocity)
{
  if (pitch < 0 || pitch >= pitchCount || m_velocities[pitch] == velocity)
    return;
  m_velocities[pitch] = velocity;
  emit velocitiesChanged();
}
} // namespace dgk
