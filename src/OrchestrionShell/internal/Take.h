/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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

#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "OrchestrionSequencer/IStaffModes.h"

#include <QString>
#include <array>
#include <optional>
#include <vector>

namespace dgk
{
/**
 * A take: what the performer played from where they started, until they went
 * somewhere else or changed a setting — for replaying it as it was played.
 */
struct Take
{
  struct Event
  {
    enum class Kind
    {
      noteOn,
      noteOff,
      sustainPedal,
      /** The automatic playback started or stopped. */
      playbackStarted,
      playbackStopped,
    };
    /** Since the take's first event. */
    double ms = 0.0;
    Kind kind = Kind::noteOn;
    int pitch = 0;
    std::optional<float> velocity;
    bool pedalDown = false;
  };

  /** The score played, as opened. */
  QString scorePath;
  /** Where the take starts: the tick, without repeats, the hands went to. */
  int startTick = 0;
  /** The bar it starts in, for showing. */
  int startBar = 1;
  std::array<StaffMode, 2> staffModes{StaffMode::rhythm, StaffMode::rhythm};
  OrnamentMode ornamentMode = OrnamentMode::automatic;
  bool loopEnabled = false;
  int loopInTick = 0;
  int loopOutTick = 0;
  std::vector<Event> events;

  int NoteCount() const;
  /** How long it lasts, in milliseconds. */
  double DurationMs() const;
};

QByteArray ToJson(const Take &);
std::optional<Take> TakeFromJson(const QByteArray &);
} // namespace dgk
