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

namespace dgk
{
PlaybackButtonModel::PlaybackButtonModel(QObject *parent) : QObject(parent) {}

void PlaybackButtonModel::load()
{
  playbackController()->isPlayingChanged().onNotify(
      this, [this] { emit isPlayingChanged(); });

  playbackController()->isPlayAllowedChanged().onNotify(
      this, [this] { emit isPlayAllowedChanged(); });
}

bool PlaybackButtonModel::isPlaying() const
{
  return playbackController()->isPlaying();
}

bool PlaybackButtonModel::isPlayAllowed() const
{
  return playbackController()->isPlayAllowed();
}

void PlaybackButtonModel::togglePlay() { dispatcher()->dispatch("play"); }

void PlaybackButtonModel::stop() { dispatcher()->dispatch("stop"); }

void PlaybackButtonModel::rewind() { dispatcher()->dispatch("rewind"); }

void PlaybackButtonModel::backStep()
{
  dispatcher()->dispatch("notation-move-left");
}

void PlaybackButtonModel::forwardStep()
{
  dispatcher()->dispatch("notation-move-right");
}
} // namespace dgk
