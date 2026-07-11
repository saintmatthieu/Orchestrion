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
#include "LoopBoundariesController.h"
#include "MuseScoreShell/OrchestrionActionIds.h"
#include <engraving/dom/chordrest.h>

namespace dgk
{
namespace
{
const mu::engraving::ChordRest *
findChordRest(const mu::notation::EngravingItem *item)
{
  const mu::engraving::EngravingItem *element = item;
  while (element && !element->isChordRest())
    element = element->parentItem();
  return element ? mu::engraving::toChordRest(element) : nullptr;
}
} // namespace

void LoopBoundariesController::init()
{
  interactionProcessor()->itemClicked().onReceive(
      this, [this](const mu::notation::EngravingItem *item)
      { m_lastClicked = chordTicks(item); });

  dispatcher()->reg(this, actionIds::loopIn,
                    [this]
                    {
                      if (m_lastClicked)
                        setLoopStart(m_lastClicked->start);
                    });
  dispatcher()->reg(this, actionIds::loopOut,
                    [this]
                    {
                      if (m_lastClicked)
                        setLoopEnd(m_lastClicked->end);
                    });
}

std::optional<ILoopBoundariesController::ChordTicks>
LoopBoundariesController::chordTicks(
    const mu::notation::EngravingItem *item) const
{
  if (!item)
    return std::nullopt;
  if (const auto chordRest = findChordRest(item))
    return ChordTicks{chordRest->tick().ticks(),
                      (chordRest->tick() + chordRest->actualTicks()).ticks()};
  const auto tick = item->tick().ticks();
  return ChordTicks{tick, tick};
}

void LoopBoundariesController::onChordShiftClicked(const ChordTicks &ticks)
{
  const auto playback = this->playback();
  if (!playback)
    return;
  if (m_awaitingLoopEnd && ticks.start > playback->loopBoundaries().loopInTick)
    setLoopEnd(ticks.end);
  else
    setLoopStart(ticks.start);
}

void LoopBoundariesController::setLoopStart(int tick)
{
  const auto playback = this->playback();
  if (!playback)
    return;
  playback->addLoopBoundary(mu::notation::LoopBoundaryType::LoopIn, tick);
  playback->setLoopBoundariesEnabled(true);
  m_awaitingLoopEnd = true;
}

void LoopBoundariesController::setLoopEnd(int endTick)
{
  const auto playback = this->playback();
  if (!playback)
    return;
  playback->addLoopBoundary(mu::notation::LoopBoundaryType::LoopOut, endTick);
  playback->setLoopBoundariesEnabled(true);
  m_awaitingLoopEnd = false;
}

void LoopBoundariesController::clearLoop()
{
  const auto playback = this->playback();
  if (!playback)
    return;
  // Zero both boundaries (an out tick at or before the in tick resets the in
  // tick to zero) so the next plain loop toggle sets up a whole-score loop
  // rather than resurrecting the cleared range.
  playback->addLoopBoundary(mu::notation::LoopBoundaryType::LoopOut, 0);
  playback->setLoopBoundariesEnabled(false);
  m_awaitingLoopEnd = false;
}

mu::notation::INotationPlaybackPtr LoopBoundariesController::playback() const
{
  const auto masterNotation = globalContext()->currentMasterNotation();
  return masterNotation ? masterNotation->playback() : nullptr;
}
} // namespace dgk
