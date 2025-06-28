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
#include "OrchestrionSequencerFactory.h"
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
        auto sequencer =
            OrchestrionSequencerFactory{}.CreateSequencer(*masterNotation);

        // This goes beyond just the official API of the audio module. Is there
        // a better way?
        muse::async::Async::call(
            this, [this]
            { audioEngine()->setMode(muse::audio::RenderMode::RealTimeMode); },
            muse::audio::AudioThread::ID);

        setSequencer(std::move(sequencer));
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
} // namespace dgk