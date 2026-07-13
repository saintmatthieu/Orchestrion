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

#include "AutomaticOrchestrionPlayer.h"
#include "IModifiableItemRegistry.h"
#include "IOrchestrion.h"
#include "IOrchestrionSequencerConfiguration.h"
#include "OrchestrionSequencer.h"
#include "ScoreAnimation/ISegmentRegistry.h"
#include "playback/iplaybackcontroller.h"
#include <async/asyncable.h>
#include <audio/internal/worker/iaudioengine.h>
#include <context/iglobalcontext.h>

namespace mu::engraving
{
class MasterScore;
}

namespace dgk
{
class Orchestrion : public IOrchestrion,
                    public muse::async::Asyncable,
                    public muse::Injectable
{
  muse::Inject<mu::playback::IPlaybackController> playbackController;
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<muse::audio::IAudioEngine> audioEngine;
  muse::Inject<ISegmentRegistry> segmentRegistry;
  muse::Inject<IOrchestrionSequencerConfiguration> sequencerConfig;

public:
  void init();

private:
  IOrchestrionSequencerPtr sequencer() override;
  muse::async::Notification sequencerChanged() const override;
  void setSequencer(IOrchestrionSequencerPtr sequencer);
  IModifiableItemRegistryPtr modifiableItemRegistry() const override;
  void setReplayTake(std::optional<ReplayTake> take) override;
  bool isReplaying() const override;
  PlayMode playMode() const override;
  void setPlayMode(PlayMode mode) override;
  muse::async::Notification playModeChanged() const override;

private:
  IOrchestrionSequencerPtr m_sequencer;
  IModifiableItemRegistryPtr m_modifiableItemRegistry;
  std::unique_ptr<AutomaticOrchestrionPlayer> m_autoPlayer;
  muse::async::Notification m_sequencerChanged;
  // The master score whose unroll decision was already taken: the choice is
  // made once per loaded score (mid-session unrolling would pull engraved
  // items out from under the review marks), and re-fires of the handler
  // must not revisit it.
  const mu::engraving::MasterScore *m_unrollDecided = nullptr;
  PlayMode m_playMode = PlayMode::replayPerformance;
  muse::async::Notification m_playModeChanged;
};
} // namespace dgk