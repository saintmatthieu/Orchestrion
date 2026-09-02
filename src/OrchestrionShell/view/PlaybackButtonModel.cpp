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
#include "PlaybackButtonModel.h"
#include "MuseScoreShell/OrchestrionActionIds.h"

namespace dgk
{
PlaybackButtonModel::PlaybackButtonModel(QObject *parent) : QObject(parent) {}

void PlaybackButtonModel::load()
{
  // Orchestrion's own playing state (the automatic player's), not
  // MuseScore's transport: the transport's playhead auto-stops at *its*
  // score end, which says nothing about our event scheduling. The player
  // lives and dies with the sequencer, so resubscribe on each swap.
  const auto subscribeToPlayer = [this]
  {
    orchestrion()->player()->PlayingChanged().onNotify(
        this, [this] { emit isPlayingChanged(); });
    // The swap itself may have changed the state (a playing player died).
    emit isPlayingChanged();
  };
  orchestrion()->sequencerChanged().onNotify(this, subscribeToPlayer);
  subscribeToPlayer();

  playbackController()->isPlayAllowedChanged().onReceive(
      this, [this](bool) { emit isPlayAllowedChanged(); });

  playbackController()->loopEnabledChanged().onReceive(
      this, [this](bool) { emit isLoopEnabledChanged(); });
}

bool PlaybackButtonModel::isPlaying() const
{
  return orchestrion()->player()->IsPlaying();
}

bool PlaybackButtonModel::isPlayAllowed() const
{
  return playbackController()->isPlayAllowed();
}

bool PlaybackButtonModel::isLoopEnabled() const
{
  return playbackController()->isLoopEnabled();
}

void PlaybackButtonModel::togglePlay()
{
  dispatcher()->dispatch(actionIds::playbackToggle);
}

void PlaybackButtonModel::stop()
{
  dispatcher()->dispatch(actionIds::playbackStop);
}

void PlaybackButtonModel::rewind() { dispatcher()->dispatch("rewind"); }

void PlaybackButtonModel::backStep() { dispatcher()->dispatch("prev"); }

void PlaybackButtonModel::forwardStep() { dispatcher()->dispatch("next"); }

void PlaybackButtonModel::toggleLoop() { dispatcher()->dispatch("loop"); }
} // namespace dgk
