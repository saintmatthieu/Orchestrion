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

#include "RecordingTimeline.h"

namespace dgk
{
namespace
{
constexpr int quarter = 480;

/**
 * Four quarter beats a second apart, then (after a skipped repeat of eight
 * beats) two more, half a second apart: beats 0-3, then 12-13.
 */
RecordingTimeline WithSkip()
{
  std::vector<RecordingBeat> beats;
  for (int b = 0; b < 4; ++b)
    beats.push_back({b * quarter, b * quarter, quarter, 1.0 + b});
  beats.push_back({12 * quarter, 4 * quarter, quarter, 6.0, true});
  beats.push_back({13 * quarter, 5 * quarter, quarter, 6.5});
  return RecordingTimeline{beats};
}

TEST(RecordingTimelineTests, InterpolatesWithinBeats)
{
  const auto timeline = WithSkip();
  ASSERT_FALSE(timeline.Empty());
  EXPECT_FALSE(timeline.PositionAt(0.5).has_value());
  EXPECT_EQ(timeline.PositionAt(1.0)->tick, 0);
  EXPECT_EQ(timeline.PositionAt(1.5)->tick, quarter / 2);
  EXPECT_EQ(timeline.PositionAt(2.25)->tick, quarter + quarter / 4);
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(quarter / 2), 1.5);
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(2 * quarter), 3.0);
}

TEST(RecordingTimelineTests, SkipsWhatTheRecordingSkips)
{
  const auto timeline = WithSkip();
  // Beat 3 is played (from 4 s, spread until the recording goes on at 6 s);
  // beats 4 to 11 aren't.
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(3 * quarter), 4.0);
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(3 * quarter + quarter / 2), 5.0);
  EXPECT_FALSE(timeline.SecondsAt(6 * quarter).has_value());
  const RecordingBeat *next = timeline.NextPlayed(6 * quarter);
  ASSERT_NE(next, nullptr);
  EXPECT_EQ(next->tick, 12 * quarter);
  EXPECT_DOUBLE_EQ(next->seconds, 6.0);
  // After the skip, in the pass played.
  const auto position = timeline.PositionAt(6.25);
  ASSERT_TRUE(position.has_value());
  EXPECT_EQ(position->tick, 12 * quarter + quarter / 2);
  EXPECT_EQ(position->writtenTick, 4 * quarter + quarter / 2);
  // The last beat lasts as long as the one before it.
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(13 * quarter + quarter / 2), 6.75);
  EXPECT_FALSE(timeline.SecondsAt(15 * quarter).has_value());
  EXPECT_EQ(timeline.NextPlayed(14 * quarter), nullptr);
}

TEST(RecordingTimelineTests, TimesGraceNotesBeforeTheBeat)
{
  const auto timeline = WithSkip();
  // Before the first beat, and before the beat after the skip: at the pace
  // of the beat they lead to.
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(-quarter / 4), 0.75);
  EXPECT_DOUBLE_EQ(*timeline.SecondsAt(12 * quarter - quarter / 4), 5.875);
}

TEST(RecordingTimelineTests, TellsThePaceAround)
{
  const auto timeline = WithSkip();
  // A quarter a second, then (after the skip) half a second.
  EXPECT_DOUBLE_EQ(*timeline.SecondsPerTick(quarter / 2), 1.0 / quarter);
  EXPECT_DOUBLE_EQ(*timeline.SecondsPerTick(-quarter), 1.0 / quarter);
  // Where it isn't played (the part skipped, past the last beat), the bars
  // unknown: the usual pace, not the last.
  EXPECT_DOUBLE_EQ(*timeline.SecondsPerTick(6 * quarter), 1.0 / quarter);
  EXPECT_DOUBLE_EQ(*timeline.SecondsPerTick(13 * quarter), 1.0 / quarter);
  EXPECT_FALSE(RecordingTimeline({{0, 0, quarter, 1.0}}).SecondsPerTick(0));
}

TEST(RecordingTimelineTests, TakesTheOtherPassOfWhatIsSkipped)
{
  // Bars A B A B of four beats, as WithSkip plays them: the first A, then
  // the second B; the first B and the second A are skipped.
  const int bar = 4 * quarter;
  const RecordingTimeline timeline{
      WithSkip().Beats(),
      {{0, 0, bar}, {bar, bar, bar}, {2 * bar, 0, bar}, {3 * bar, bar, bar}}};
  // The second A, as the first is played; the first B's first beat, as the
  // second's is.
  EXPECT_DOUBLE_EQ(*timeline.SecondsPerTick(2 * bar + quarter), 1.0 / quarter);
  EXPECT_DOUBLE_EQ(*timeline.SecondsPerTick(bar), 0.5 / quarter);
  // The rest of the first B: its other pass is played no further; and past
  // the end. The usual pace.
  EXPECT_DOUBLE_EQ(*timeline.SecondsPerTick(bar + 2 * quarter), 1.0 / quarter);
  EXPECT_DOUBLE_EQ(*timeline.SecondsPerTick(5 * bar), 1.0 / quarter);
}

TEST(RecordingTimelineTests, SmoothsTheNoiseOfTaps)
{
  // Steady beats a second apart, the third tapped 150 ms late; a skip
  // before the last two.
  auto beats = WithSkip().Beats();
  beats[2].seconds += 0.15;
  const auto smoothed = SmoothBeats(beats);
  EXPECT_DOUBLE_EQ(smoothed[0].seconds, 1.0); // a run's ends stay
  EXPECT_DOUBLE_EQ(smoothed[1].seconds, 2.05);
  EXPECT_DOUBLE_EQ(smoothed[2].seconds, 3.05);
  EXPECT_DOUBLE_EQ(smoothed[3].seconds, 4.0);
  EXPECT_DOUBLE_EQ(smoothed[4].seconds, 6.0);
  EXPECT_DOUBLE_EQ(smoothed[5].seconds, 6.5);
  // Steady beats stay.
  const auto steady = WithSkip().Beats();
  const auto unchanged = SmoothBeats(steady, 3);
  for (size_t i = 0; i < steady.size(); ++i)
    EXPECT_DOUBLE_EQ(unchanged[i].seconds, steady[i].seconds) << i;
}

TEST(RecordingTimelineTests, RefusesBeatsOutOfOrder)
{
  EXPECT_TRUE(RecordingTimeline(
                  {{0, 0, quarter, 1.0}, {quarter, quarter, quarter, 0.5}})
                  .Empty());
  EXPECT_TRUE(RecordingTimeline(
                  {{quarter, quarter, quarter, 1.0}, {0, 0, quarter, 2.0}})
                  .Empty());
}
} // namespace
} // namespace dgk
