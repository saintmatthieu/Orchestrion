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
namespace
{
// How long a lift stays on screen at least. The pedal itself stays up 100 ms
// on a re-pedal, but the lift and the press may reach the GUI thread in one
// batch, and the slide takes time to read.
constexpr auto minLiftShown = std::chrono::milliseconds{150};
} // namespace

PedalIndicatorModel::PedalIndicatorModel(QObject *parent) : QObject(parent)
{
  m_pressTimer.setSingleShot(true);
  connect(&m_pressTimer, &QTimer::timeout, this,
          [this] { setPedalDown(true); });
}

void PedalIndicatorModel::load()
{
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this] { followSequencer(); });
  followSequencer();

  sequencerConfiguration()->pedalIndicatorVisibleChanged().onNotify(
      this, [this] { emit iconVisibleChanged(); });
  // The view's binding was evaluated before we subscribed.
  emit iconVisibleChanged();
}

void PedalIndicatorModel::followSequencer()
{
  // A sequencer starts with the pedal up, and the one it replaces lifts it
  // on its way out.
  onPedalLifted();
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
          pedal->on ? onPedalPressed() : onPedalLifted();
      },
      muse::async::Asyncable::Mode::SetReplace);
}

void PedalIndicatorModel::onPedalLifted()
{
  m_pressTimer.stop();
  m_liftShownAt = std::chrono::steady_clock::now();
  setPedalDown(false);
}

void PedalIndicatorModel::onPedalPressed()
{
  using namespace std::chrono;
  // Not before the lift has been seen.
  const auto remaining = m_liftShownAt + minLiftShown - steady_clock::now();
  if (remaining > 0ms)
    m_pressTimer.start(duration_cast<milliseconds>(remaining));
  else
    setPedalDown(true);
}

bool PedalIndicatorModel::pedalDown() const { return m_pedalDown; }

bool PedalIndicatorModel::iconVisible() const
{
  return sequencerConfiguration()->pedalIndicatorVisible();
}

void PedalIndicatorModel::setPedalDown(bool down)
{
  if (down == m_pedalDown)
    return;
  m_pedalDown = down;
  emit pedalDownChanged();
}
} // namespace dgk
