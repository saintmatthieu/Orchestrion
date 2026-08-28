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

#include "IOrchestrionPlayer.h"

namespace dgk
{
//! The player IOrchestrion::player() hands out while no real one exists
//! (no score loaded yet): never playing, never replaying, never notifying —
//! so consumers need no null checks.
class OrchestrionPlayerStub : public IOrchestrionPlayer
{
public:
  void SetReplayTake(std::optional<ReplayTake>) override {}
  bool IsReplaying() const override { return false; }
  bool IsPlaying() const override { return false; }
  muse::async::Notification PlayingChanged() const override
  {
    return m_playingChanged;
  }

private:
  muse::async::Notification m_playingChanged;
};
} // namespace dgk
