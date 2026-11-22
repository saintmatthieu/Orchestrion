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

#include "OrchestrionSequencer/RecordingTimeline.h"

#include <QString>
#include <memory>
#include <optional>
#include <vector>

namespace mu::engraving
{
class Score;
}

namespace dgk
{
/**
 * When a recording of a score plays its beats, tapped by someone watching
 * it: the sync file of a recording.
 *
 * The file is JSON: {"version": 2, "score": "<the score's Rolls version>",
 * "points": [[bar, tick, seconds], ...]}. `bar` is the measure's index as
 * the music is played (repeats unrolled, from 0, a pickup included); `tick`
 * where in it the point is, in ticks from its start (a beat or one of its
 * subdivisions, as tapped); `seconds` from the start of the piece in the
 * recording (its link's t=). The points are in the order played; a bar
 * index that jumps by more than one is a part of the score the recording
 * skips (a repeat). Version 1 had the beat, from 1, instead of the tick.
 */
struct RecordingSync
{
  struct Point
  {
    int bar = 0;
    int tick = 0;
    double seconds = 0.0;
  };

  /** The version of the score the points were tapped on. */
  QString scoreVersion;
  std::vector<Point> points;
};

/** A bar as the music is played: repeats unrolled. */
struct PlayedBar
{
  /** Where it starts, in ticks with repeats, and as written. */
  int tick = 0;
  int writtenTick = 0;
  /**
   * Its length, its beat's (the time signature's: a dotted quarter in 12/8)
   * and its beat's subdivision's (an eighth in 12/8, in 4/4 too), in ticks.
   */
  int length = 0;
  int beatTicks = 0;
  int subdivisionTicks = 0;
  /** How many beats it has, the last maybe shorter. */
  int beats = 0;
  /** Its number as printed; a pickup is the first bar shorter than its time. */
  int number = 0;
  bool pickup = false;
  /** Which time it is played, from 1: a repeat's second time is 2. */
  int pass = 1;
};

std::vector<PlayedBar> PlayedBars(const mu::engraving::Score &);

/** Where the sync file of a recording is kept. */
QString RecordingSyncPath(const QString &userDataPath, const QString &pieceId,
                          const QString &videoId);

/** `bars` are the score's, as played, for a file of version 1. */
std::optional<RecordingSync>
ReadRecordingSync(const QString &path, const std::vector<PlayedBar> &bars);
bool WriteRecordingSync(const QString &path, const RecordingSync &);

/**
 * The points as the score's positions: none if some point isn't in it (a bar
 * or a beat past its end), or if they aren't in the order played.
 */
std::shared_ptr<const RecordingTimeline>
MakeRecordingTimeline(const mu::engraving::Score &, const RecordingSync &);
} // namespace dgk
