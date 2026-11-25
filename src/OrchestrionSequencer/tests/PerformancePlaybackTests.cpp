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
#include <gtest/gtest.h>

#include "internal/PerformancePlayback.h"

namespace dgk
{
namespace
{
constexpr int quarter = 480;
constexpr double nominalTicksPerMs = 2.0 * quarter / 1000.0;

int CMajor(int degree)
{
  static constexpr int steps[] = {0, 2, 4, 5, 7, 9, 11};
  return 60 + 12 * (degree / 7) + steps[degree % 7];
}

/**
 * |: C D E F G A B C :| D C B A G F E D, the repeat not played: a quarter
 * every half second, louder and louder.
 */
struct SkippedRepeat
{
  std::vector<ScoreEvent> events;
  std::vector<PerformedNote> notes;

  SkippedRepeat()
  {
    for (int pass = 0; pass < 2; ++pass)
      for (int i = 0; i < 8; ++i)
        events.push_back({(8 * pass + i) * quarter, {CMajor(i)}, i * quarter});
    for (int i = 0; i < 8; ++i)
      events.push_back(
          {(16 + i) * quarter, {CMajor(8 - i)}, (8 + i) * quarter});
    for (int i = 0; i < 16; ++i)
      notes.push_back({1000.0 + 500.0 * i, 1000.0 + 500.0 * i + 400.0,
                       events[i < 8 ? i : i + 8].pitches[0], 40 + 4 * i});
  }
};

TEST(PerformancePlaybackTests, TimesTheScoreAsPerformed)
{
  const SkippedRepeat performance;
  const auto playback = MakePerformancePlayback(
      performance.events, performance.notes,
      AlignPerformance(performance.events, performance.notes,
                       nominalTicksPerMs));
  ASSERT_TRUE(playback.timeline);
  const RecordingTimeline &timeline = *playback.timeline;
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(0), 1.0);
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(3 * quarter + quarter / 2), 2.75);
  // The second pass is skipped: not played, yet at the first's pace.
  EXPECT_FALSE(timeline.SecondsAt(10 * quarter).has_value());
  EXPECT_DOUBLE_EQ(*timeline.SecondsPerTick(10 * quarter), 0.5 / quarter);
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(16 * quarter), 5.0);
}

TEST(PerformancePlaybackTests, GivesEachNoteItsVelocity)
{
  const SkippedRepeat performance;
  const auto playback = MakePerformancePlayback(
      performance.events, performance.notes,
      AlignPerformance(performance.events, performance.notes,
                       nominalTicksPerMs));
  ASSERT_TRUE(playback.velocities);
  EXPECT_FLOAT_EQ(*playback.velocities->At(quarter, CMajor(1)), 44 / 127.f);
  EXPECT_FLOAT_EQ(*playback.velocities->At(16 * quarter, CMajor(8)),
                  72 / 127.f);
  // Not performed: the second pass, a pitch not there.
  EXPECT_FALSE(playback.velocities->At(9 * quarter, CMajor(1)).has_value());
  EXPECT_FALSE(playback.velocities->At(quarter, CMajor(2)).has_value());
}

TEST(PerformancePlaybackTests, NothingForAnAlignmentOfOtherEvents)
{
  const SkippedRepeat performance;
  const auto playback = MakePerformancePlayback(
      performance.events, performance.notes, PerformanceAlignment{});
  EXPECT_FALSE(playback.timeline);
  EXPECT_FALSE(playback.velocities);
}
} // namespace
} // namespace dgk
