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
#include "PedalIndicatorModel.h"

#include <variant>

namespace dgk
{
PedalIndicatorModel::PedalIndicatorModel(QObject *parent) : QObject(parent) {}

void PedalIndicatorModel::load()
{
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this] { followSequencer(); });
  followSequencer();
}

void PedalIndicatorModel::followSequencer()
{
  // A sequencer starts with the pedal up, and the one it replaces lifts it
  // on its way out.
  setPedalDown(false);
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  // The sequencer sends from its own threads; the channel delivers on the
  // thread that subscribed (this one, the GUI's), whose queue the framework
  // drains from its ticker.
  sequencer->OutputEvent().onReceive(
      this,
      [this](const EventVariant &event)
      {
        if (const auto *pedal = std::get_if<PedalEvent>(&event))
          setPedalDown(pedal->on);
      },
      muse::async::Asyncable::Mode::SetReplace);
}

bool PedalIndicatorModel::pedalDown() const { return m_pedalDown; }

void PedalIndicatorModel::setPedalDown(bool down)
{
  if (down == m_pedalDown)
    return;
  m_pedalDown = down;
  emit pedalDownChanged();
}
} // namespace dgk
