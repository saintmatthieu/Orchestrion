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
#include "TempoModel.h"
#include "OrchestrionSequencer/IChord.h"
#include "OrchestrionSequencer/IOrchestrionSequencer.h"
#include "OrchestrionSequencer/OrchestrionTypes.h"

#include <engraving/dom/masterscore.h>
#include <engraving/dom/tempotimeline.h>
#include <notation/imasternotation.h>

#include <algorithm>
#include <climits>
#include <cmath>

namespace dgk
{
TempoModel::TempoModel(QObject *parent) : QObject(parent) {}

void TempoModel::load()
{
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this] { followSequencer(); });
  followSequencer();
  tempoMultiplier()->multiplierChanged().onNotify(this,
                                                  [this] { emit changed(); });
  globalContext()->currentMasterNotationChanged().onNotify(
      this, [this] { emit changed(); });
}

void TempoModel::followSequencer()
{
  m_tick = 0;
  emit changed();
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  sequencer->ChordTransitions().onReceive(
      this,
      [this](const std::map<TrackIndex, ChordTransition> &transitions)
      {
        // Where the hands that moved are: the earliest chord they sound or
        // are about to.
        int tick = INT_MAX;
        for (const auto &[track, transition] : transitions)
          if (const IChord *chord = GetPresentChord(transition)
                                        ? GetPresentChord(transition)
                                        : GetFutureChord(transition))
            tick = std::min(tick, chord->GetBeginTick().withRepeats);
        if (tick == INT_MAX || tick == m_tick)
          return;
        const int before = bpm();
        m_tick = tick;
        if (bpm() != before)
          emit changed();
      },
      muse::async::Asyncable::Mode::SetReplace);
}

double TempoModel::markedBpm() const
{
  const auto master = globalContext()->currentMasterNotation();
  if (!master || !master->masterScore())
    return 0.0;
  return master->masterScore()
             ->tempoTimeline(/*expandRepeats*/ true)
             .tempo(m_tick)
             .val *
         60.0;
}

bool TempoModel::available() const { return markedBpm() > 0.0; }

int TempoModel::bpm() const
{
  return static_cast<int>(std::lround(markedBpm() * multiplier()));
}

int TempoModel::minBpm() const
{
  return static_cast<int>(std::ceil(markedBpm() * ITempoMultiplier::min));
}

int TempoModel::maxBpm() const
{
  return static_cast<int>(std::floor(markedBpm() * ITempoMultiplier::max));
}

double TempoModel::multiplier() const
{
  return tempoMultiplier()->multiplier();
}

void TempoModel::setBpm(int value)
{
  const double marked = markedBpm();
  if (marked <= 0.0 || value <= 0)
    return;
  tempoMultiplier()->setMultiplier(value / marked);
}

void TempoModel::reset() { tempoMultiplier()->setMultiplier(1.0); }
} // namespace dgk
