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
#include "IChord.h"
#include "OrchestrionSequencerFactory.h" // NotationProducts
#include <async/async.h>
#include <audio/internal/audiothread.h>
#include <engraving/dom/masterscore.h>

namespace dgk
{
void Orchestrion::init()
{
  playbackController()->isPlayAllowedChanged().onNotify(
      this,
      [&]()
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
        // reads it (a toggle change applies to the next loaded score).
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

        // This goes beyond just the official API of the audio module. Is there
        // a better way?
        muse::async::Async::call(
            this, [this]
            { audioEngine()->setMode(muse::audio::RenderMode::RealTimeMode); },
            muse::audio::AudioThread::ID);

        m_modifiableItemRegistry = products.modifiableItemRegistry;
        if (products.sequencer)
          m_autoPlayer =
              std::make_unique<AutomaticOrchestrionPlayer>(*products.sequencer);
        else
          m_autoPlayer.reset();
        setSequencer(products.sequencer);

        if (const auto notation = globalContext()->currentMasterNotation())
          notation->masterScore()->setSaved(true);
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

void Orchestrion::setReplayTake(std::optional<ReplayTake> take)
{
  if (m_autoPlayer)
    m_autoPlayer->SetReplayTake(std::move(take));
}

bool Orchestrion::isReplaying() const
{
  return m_autoPlayer && m_autoPlayer->IsReplaying();
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