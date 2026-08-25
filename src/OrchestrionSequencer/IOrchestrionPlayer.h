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

#include <async/notification.h>
#include <memory>

namespace dgk
{
//! The automatic player: machine-driven gesture events playing the score.
//! Owns Orchestrion's playing state, driven by the orchestrion-play/-stop
//! actions and deliberately independent of MuseScore's transport (whose
//! playhead runs over the silent notation tracks and auto-stops at *its*
//! score end — Orchestrion playback is just scheduled gesture events).
class IOrchestrionPlayer
{
public:
  virtual ~IOrchestrionPlayer() = default;

  //! Whether the player is producing events.
  virtual bool IsPlaying() const = 0;
  virtual muse::async::Notification PlayingChanged() const = 0;
};

using IOrchestrionPlayerPtr = std::shared_ptr<IOrchestrionPlayer>;
} // namespace dgk
