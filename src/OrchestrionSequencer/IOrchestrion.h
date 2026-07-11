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

#include "IOrchestrionSequencer.h"
#include "IModifiableItemRegistry.h"
#include <async/notification.h>
#include <modularity/imoduleinterface.h>

namespace dgk
{
//! What the play button does while a finished take is on record: replay the
//! performance verbatim, re-perform it on its *fitted* tempo curve (the
//! spline the judgments compare against — the performance minus its per-note
//! jitter), or the plain metronomic playback.
enum class PlayMode
{
  replayPerformance,
  replayFittedTempo,
  metronome,
};

class IOrchestrion : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrion);

public:
  virtual ~IOrchestrion() = default;

  virtual IOrchestrionSequencerPtr sequencer() = 0;
  virtual muse::async::Notification sequencerChanged() const = 0;
  virtual IModifiableItemRegistryPtr modifiableItemRegistry() const = 0;

  //! While set, the play button replays this recorded take — the machine
  //! re-performs it, timing, dynamics and all — instead of the metronomic
  //! automatic playback (the post-take review's listen-back). Cleared when a
  //! new take begins.
  virtual void setReplayTake(std::optional<ReplayTake> take) = 0;
  //! Whether such a replay is running right now. Its re-injected events must
  //! not be judged as a new performance.
  virtual bool isReplaying() const = 0;

  //! Session-only (deliberately not persisted): a review/tuning aid.
  virtual PlayMode playMode() const = 0;
  virtual void setPlayMode(PlayMode mode) = 0;
  virtual muse::async::Notification playModeChanged() const = 0;
};
} // namespace dgk