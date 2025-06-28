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
#pragma once

#include "IOrchestrion.h"
#include "OrchestrionSequencer.h"
#include "playback/iplaybackcontroller.h"
#include <async/asyncable.h>
#include <audio/internal/worker/iaudioengine.h>
#include <context/iglobalcontext.h>

namespace dgk
{
class Orchestrion : public IOrchestrion,
                    public muse::async::Asyncable,
                    public muse::Injectable
{
  muse::Inject<mu::playback::IPlaybackController> playbackController = {this};
  muse::Inject<mu::context::IGlobalContext> globalContext = {this};
  muse::Inject<muse::audio::IAudioEngine> audioEngine = {this};

public:
  void init();

private:
  IOrchestrionSequencerPtr sequencer() override;
  muse::async::Notification sequencerChanged() const override;
  void setSequencer(IOrchestrionSequencerPtr sequencer);

private:
  IOrchestrionSequencerPtr m_sequencer;
  muse::async::Notification m_sequencerChanged;
};
} // namespace dgk