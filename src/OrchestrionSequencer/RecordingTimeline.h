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

#include <optional>
#include <vector>

namespace dgk
{
/** A beat of the score, and when a recording plays it. */
struct RecordingBeat
{
  /** Where the beat is, in ticks with repeats. */
  int tick = 0;
  /** The same position in the score as written. */
  int writtenTick = 0;
  /** How long the beat is, in ticks. */
  int beatTicks = 0;
  /** When the recording plays it, in seconds. */
  double seconds = 0.0;
  /**
   * Whether the recording skips some of the score before it (a repeat):
   * what lies between the beat before and this one isn't played.
   */
  bool afterSkip = false;
};

/** A bar of the score as played: where it is, with repeats and as written. */
struct RecordingBar
{
  int tick = 0;
  int writtenTick = 0;
  int length = 0;
};

/**
 * The beats with the noise of their timing (beats tapped by hand, say)
 * smoothed out: each moved a third of the way to where its neighbours,
 * played one after the other, put it, `passes` times. Steady beats stay;
 * so do the ends of each run played without a skip, and the tempo's
 * broader moves.
 */
std::vector<RecordingBeat> SmoothBeats(std::vector<RecordingBeat> beats,
                                       int passes = 1);

/** Where in the score a recording is, as it plays. */
class RecordingTimeline
{
public:
  struct Position
  {
    int tick = 0;
    int writtenTick = 0;
  };

  /**
   * `beats` in the order the recording plays them: their seconds and ticks
   * rising. Empty if they aren't. `bars`, the score's as played, tell
   * SecondsPerTick the other passes of a repeat.
   */
  explicit RecordingTimeline(std::vector<RecordingBeat> beats,
                             std::vector<RecordingBar> bars = {});

  bool Empty() const { return m_beats.empty(); }
  const std::vector<RecordingBeat> &Beats() const { return m_beats; }

  /** Where the recording is at `seconds`; none before its first beat. */
  std::optional<Position> PositionAt(double seconds) const;

  /**
   * When the recording plays `tick` (with repeats); none for a part of the
   * score it skips or doesn't reach.
   */
  std::optional<double> SecondsAt(int tick) const;

  /**
   * The first beat played at or after `tick`: where the recording goes on
   * after a part of the score it skips. Null if none.
   */
  const RecordingBeat *NextPlayed(int tick) const;

  /**
   * How fast the recording plays the score at `tick`, in seconds per tick:
   * between the beats around it; where it doesn't play it (a repeat it
   * skips, before its start, past its end), as it plays another pass of the
   * same bar, else at its usual pace (the median), not slowed by its
   * ritardandi. None if no two beats are played one after the other.
   */
  std::optional<double> SecondsPerTick(int tick) const;

private:
  /** How far the music played from beat `i` reaches before the next beat. */
  int ReachedTicks(size_t i) const;
  /** The pace between the beats played around `tick`, if it is played. */
  std::optional<double> PlayedPace(int tick) const;

  std::vector<RecordingBeat> m_beats;
  std::vector<RecordingBar> m_bars;
  std::optional<double> m_usualPace;
};
} // namespace dgk
