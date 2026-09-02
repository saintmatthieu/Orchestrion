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
#include "Orchestrion.h"
#include <engraving/editing/transaction/undostack.h>
#include <notation/imasternotation.h>
#include "IChord.h"
#include "OrchestrionPlayerStub.h"
#include "OrchestrionSequencerFactory.h" // NotationProducts
#include <async/async.h>
#include <cassert>
#include <engraving/dom/masterscore.h>

namespace dgk
{
void Orchestrion::init()
{
  // Unrolling repeats only serves the grading visuals (ribbon, beat grid,
  // layout warp), so for now it is not a choice of its own: it follows the
  // grading switch. The setting survives as the place to make it
  // configurable again — drop this sync and give it back its menu item.
  const auto syncUnrollRepeats = [this]
  {
    sequencerConfig()->setUnrollRepeatsEnabled(
        sequencerConfig()->gradingEnabled());
  };
  sequencerConfig()->gradingEnabledChanged().onNotify(this, syncUnrollRepeats);
  syncUnrollRepeats();

  playbackController()->isPlayAllowedChanged().onReceive(
      this,
      [&](bool)
      {
        const auto masterNotation = globalContext()->currentMasterNotation();
        if (!masterNotation)
        {
          setSequencer(nullptr);
          return;
        }

        // Unroll the score's repeats into the score itself, so every pass is
        // its own engraved passage: the deviation ribbon, beat grid and
        // layout warp then carry through repeats without folding passes onto
        // the same bars. Decided once per loaded score, before the sequencer
        // reads it (so switching grading applies at the next loaded score).
        if (mu::engraving::MasterScore *const master =
                masterNotation->masterScore();
            master && master != m_unrollDecided)
        {
          m_unrollDecided = master;
          if (sequencerConfig()->unrollRepeatsEnabled())
            master->unrollRepeatsInPlace();
        }

        const NotationProducts products =
            OrchestrionSequencerFactory{}.CreateSequencer(*masterNotation);

        m_modifiableItemRegistry = products.modifiableItemRegistry;
        if (products.sequencer)
          m_autoPlayer =
              std::make_shared<AutomaticOrchestrionPlayer>(*products.sequencer);
        else
          m_autoPlayer.reset();
        setSequencer(products.sequencer);

        // Unrolling the repeats is not a modification of the user's score.
        if (const auto notation = globalContext()->currentMasterNotation())
          notation->masterScore()->undoStack()->markClean();
      });
}

void Orchestrion::setSequencer(IOrchestrionSequencerPtr sequencer)
{
  if (sequencer == m_sequencer)
    return;
  m_sequencer = std::move(sequencer);


  m_sequencerChanged.notify();
}

IOrchestrionSequencerPtr Orchestrion::sequencer() { return m_sequencer; }

muse::async::Notification Orchestrion::sequencerChanged() const
{
  return m_sequencerChanged;
}

IModifiableItemRegistryPtr Orchestrion::modifiableItemRegistry() const
{
  return m_modifiableItemRegistry;
}

IOrchestrionPlayerPtr Orchestrion::player()
{
  if (m_autoPlayer)
    return m_autoPlayer;
  // No player before a score is loaded — normal, the stub covers it. But a
  // live sequencer without its player is a broken invariant (they are
  // created and destroyed together).
  assert(!m_sequencer);
  static const IOrchestrionPlayerPtr stub =
      std::make_shared<OrchestrionPlayerStub>();
  return stub;
}

PlayMode Orchestrion::playMode() const { return m_playMode; }

void Orchestrion::setPlayMode(PlayMode mode)
{
  if (mode == m_playMode)
    return;
  m_playMode = mode;
  m_playModeChanged.notify();
}

muse::async::Notification Orchestrion::playModeChanged() const
{
  return m_playModeChanged;
}
} // namespace dgk