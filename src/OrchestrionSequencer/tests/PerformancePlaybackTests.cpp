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
  ASSERT_TRUE(playback.performed);
  const PerformedScore &performed = *playback.performed;
  EXPECT_FLOAT_EQ(*performed.Velocity(quarter, CMajor(1)), 44 / 127.f);
  EXPECT_FLOAT_EQ(*performed.Velocity(16 * quarter, CMajor(8)), 72 / 127.f);
  // Not performed: the second pass, a pitch not there.
  EXPECT_FALSE(performed.Velocity(9 * quarter, CMajor(1)).has_value());
  EXPECT_FALSE(performed.Velocity(quarter, CMajor(2)).has_value());
  // Half a second a quarter.
  EXPECT_DOUBLE_EQ(*performed.TicksPerMsAt(quarter), quarter / 500.0);
}

TEST(PerformancePlaybackTests, LetsGoOfAChordStruckAgainWhenTheLastEnds)
{
  // A half-note C, then E: the performer strikes the C again, briefly
  // first, then held, the second time matched to no event.
  const std::vector<ScoreEvent> events{{0, {60}, 0},
                                       {2 * quarter, {64}, 2 * quarter}};
  const std::vector<PerformedNote> notes{{1000.0, 1200.0, 60, 60},
                                         {1300.0, 1950.0, 60, 60},
                                         {2000.0, 2500.0, 64, 60}};
  const auto playback = MakePerformancePlayback(
      events, notes, AlignPerformance(events, notes, nominalTicksPerMs));
  ASSERT_TRUE(playback.performed);
  // At 1950 ms of the 1000 between the events.
  EXPECT_NEAR(*playback.performed->ReleaseTick(0, 2 * quarter, 30, {60}),
              0.95 * 2 * quarter, 1);
}

TEST(PerformancePlaybackTests, RecoversNotesPlayedOutOfOrder)
{
  // A melody note, a bass note, a melody note; the bass played ahead of
  // the first melody note, left aside by the alignment; another of its
  // pitch long after.
  const std::vector<ScoreEvent> events{
      {0, {72}, 0}, {quarter, {48}, quarter}, {2 * quarter, {74}, 2 * quarter}};
  const std::vector<PerformedNote> notes{{900.0, 1800.0, 48, 64},
                                         {1000.0, 1400.0, 72, 70},
                                         {2000.0, 2400.0, 74, 70},
                                         {6000.0, 6400.0, 48, 64}};
  PerformanceAlignment alignment;
  alignment.events.resize(3);
  alignment.events[0] = {1000.0, {1}, {}};
  alignment.events[1] = {std::nullopt, {PerformanceAlignment::none}, {}};
  alignment.events[2] = {2000.0, {2}, {}};
  alignment.added = {0, 3};
  const auto playback = MakePerformancePlayback(events, notes, alignment);
  ASSERT_TRUE(playback.performed);
  EXPECT_FALSE(playback.performed->LeftOut(quarter));
  EXPECT_DOUBLE_EQ(*playback.performed->Onset(quarter, 48), 900.0);
  EXPECT_FLOAT_EQ(*playback.performed->Velocity(quarter, 48), 64 / 127.f);
}

TEST(PerformancePlaybackTests, RecoversANotePlayedASemitoneOff)
{
  // The middle note, alone, played a semitone higher.
  const std::vector<ScoreEvent> events{
      {0, {72}, 0}, {quarter, {71}, quarter}, {2 * quarter, {74}, 2 * quarter}};
  const std::vector<PerformedNote> notes{{1000.0, 1400.0, 72, 70},
                                         {1500.0, 1900.0, 72, 60},
                                         {2000.0, 2400.0, 74, 70}};
  PerformanceAlignment alignment;
  alignment.events.resize(3);
  alignment.events[0] = {1000.0, {0}, {}};
  alignment.events[1] = {std::nullopt, {PerformanceAlignment::none}, {}};
  alignment.events[2] = {2000.0, {2}, {}};
  alignment.added = {1};
  const auto playback = MakePerformancePlayback(events, notes, alignment);
  ASSERT_TRUE(playback.performed);
  EXPECT_FALSE(playback.performed->LeftOut(quarter));
  // For the score's note, played as performed.
  EXPECT_DOUBLE_EQ(*playback.performed->Onset(quarter, 71), 1500.0);
  EXPECT_FLOAT_EQ(*playback.performed->Velocity(quarter, 71), 60 / 127.f);
  EXPECT_EQ(playback.performed->PlayedPitch(quarter, 71), 72);
  EXPECT_EQ(playback.performed->PlayedPitch(0, 72), 72);
}

TEST(PerformancePlaybackTests, TellsWhatThePerformerLeftOut)
{
  // The third note of the first pass not played.
  SkippedRepeat performance;
  performance.notes.erase(performance.notes.begin() + 2);
  const auto playback = MakePerformancePlayback(
      performance.events, performance.notes,
      AlignPerformance(performance.events, performance.notes,
                       nominalTicksPerMs));
  ASSERT_TRUE(playback.performed);
  EXPECT_TRUE(playback.performed->LeftOut(2 * quarter));
  EXPECT_FALSE(playback.performed->LeftOut(quarter));
  // Not the repeat skipped, nor past the end.
  EXPECT_FALSE(playback.performed->LeftOut(10 * quarter));
  EXPECT_FALSE(playback.performed->LeftOut(20 * quarter));
}

TEST(PerformancePlaybackTests, PlacesThePedalInTheScore)
{
  const SkippedRepeat performance;
  // Down before the first note, up and down again a quarter later (500 ms
  // a quarter), an up and a down so close together that only the down
  // stands.
  const auto playback = MakePerformancePlayback(
      performance.events, performance.notes,
      AlignPerformance(performance.events, performance.notes,
                       nominalTicksPerMs),
      {{900.0, true},
       {1500.0, false},
       {1550.0, true},
       {2500.0, false},
       {2500.2, true}});
  ASSERT_TRUE(playback.performed);
  const auto &pedal = playback.performed->Pedal();
  ASSERT_EQ(pedal.size(), 4u);
  EXPECT_EQ(pedal[0].tick, 0);
  EXPECT_TRUE(pedal[0].down);
  EXPECT_EQ(pedal[1].tick, quarter);
  EXPECT_FALSE(pedal[1].down);
  EXPECT_EQ(pedal[2].tick, quarter + quarter / 10);
  EXPECT_TRUE(pedal[2].down);
  EXPECT_EQ(pedal[3].tick, 3 * quarter);
  EXPECT_TRUE(pedal[3].down);
}

TEST(PerformancePlaybackTests, StrikesAnEventWithItsEarliestNote)
{
  // A bass and a melody note together, the melody played 200 ms after the
  // bass; then the same again a second later.
  const std::vector<ScoreEvent> events{{0, {48, 72}, 0},
                                       {2 * quarter, {48, 74}, 2 * quarter}};
  const std::vector<PerformedNote> notes{{1000.0, 1900.0, 48, 50},
                                         {1200.0, 1900.0, 72, 70},
                                         {2000.0, 2900.0, 48, 50},
                                         {2200.0, 2900.0, 74, 70}};
  const auto playback = MakePerformancePlayback(
      events, notes, AlignPerformance(events, notes, nominalTicksPerMs));
  ASSERT_TRUE(playback.performed);
  EXPECT_DOUBLE_EQ(*playback.performed->StrikeMs(0), 1000.0);
  EXPECT_DOUBLE_EQ(*playback.performed->Onset(0, 72), 1200.0);
  ASSERT_TRUE(playback.timeline);
  EXPECT_DOUBLE_EQ(*playback.timeline->SecondsAt(2 * quarter), 2.0);
}

TEST(PerformancePlaybackTests, StrikesAnEventPlayedAheadInOrder)
{
  // Three events, the second's bass played ahead of the first: it can't be
  // struck before the first, so it is struck between its neighbours.
  const std::vector<ScoreEvent> events{{0, {72}, 0},
                                       {quarter, {60, 74}, quarter},
                                       {2 * quarter, {76}, 2 * quarter}};
  const std::vector<PerformedNote> notes{{950.0, 1900.0, 60, 50},
                                         {1000.0, 1400.0, 72, 70},
                                         {1600.0, 1900.0, 74, 70},
                                         {2000.0, 2400.0, 76, 70}};
  PerformanceAlignment alignment;
  alignment.events.resize(3);
  alignment.events[0] = {1000.0, {1}, {}};
  alignment.events[1] = {950.0, {0, 2}, {}};
  alignment.events[2] = {2000.0, {3}, {}};
  const auto playback = MakePerformancePlayback(events, notes, alignment);
  ASSERT_TRUE(playback.performed);
  EXPECT_DOUBLE_EQ(*playback.performed->StrikeMs(quarter), 1500.0);
  EXPECT_DOUBLE_EQ(*playback.performed->StrikeMs(0), 1000.0);
}

TEST(PerformancePlaybackTests, NothingForAnAlignmentOfOtherEvents)
{
  const SkippedRepeat performance;
  const auto playback = MakePerformancePlayback(
      performance.events, performance.notes, PerformanceAlignment{});
  EXPECT_FALSE(playback.timeline);
  EXPECT_FALSE(playback.performed);
}

/**
 * A half-note E with a trill (F), then a quarter G; the performer trills
 * E F E F E F E, loudly first, over 600 ms, then holds E until the G.
 */
struct Trill
{
  std::vector<ScoreEvent> events;
  std::vector<PerformedNote> notes;

  Trill()
  {
    ScoreEvent trill{0, {CMajor(2)}, 0};
    trill.trillPitches = {CMajor(2), CMajor(3)};
    trill.trillEnd = 2 * quarter;
    events.push_back(trill);
    events.push_back({2 * quarter, {CMajor(4)}, 2 * quarter});
    for (int i = 0; i < 7; ++i)
      notes.push_back({1000.0 + 100.0 * i, 1000.0 + 100.0 * i + 90.0,
                       CMajor(i % 2 ? 3 : 2), i == 0 ? 80 : 60});
    notes.push_back({2000.0, 2400.0, CMajor(4), 70});
  }
};

TEST(PerformancePlaybackTests, GivesAnOrnamentItsPerformedNotes)
{
  const Trill trill;
  const auto playback = MakePerformancePlayback(
      trill.events, trill.notes,
      AlignPerformance(trill.events, trill.notes, nominalTicksPerMs));
  ASSERT_TRUE(playback.performed);
  const auto notes =
      playback.performed->NotesBetween(0, 2 * quarter, {CMajor(2), CMajor(3)});
  ASSERT_EQ(notes.size(), 7u);
  for (size_t i = 0; i < notes.size(); ++i)
    EXPECT_DOUBLE_EQ(notes[i].onsetMs, 1000.0 + 100.0 * i) << i;
  // Let go of as the last trill note was (1690 ms, of the 1000 between the
  // two events), not before a 64th, not past the end.
  EXPECT_NEAR(*playback.performed->ReleaseTick(0, 2 * quarter, 30,
                                               {CMajor(2), CMajor(3)}),
              0.69 * 2 * quarter, 1);
  EXPECT_EQ(
      *playback.performed->ReleaseTick(0, quarter, 30, {CMajor(2), CMajor(3)}),
      quarter);
  // Only those of the pitches asked for.
  EXPECT_TRUE(
      playback.performed->NotesBetween(0, 2 * quarter, {CMajor(4)}).empty());
}

TEST(PerformancePlaybackTests, PlaysAnOrnamentAsPerformed)
{
  const Trill trill;
  // At half speed, the strike as loud as the trill's first note, and with
  // it.
  const auto schedule = PerformedOrnament(
      {trill.notes.begin(), trill.notes.begin() + 7}, 1000.0, 2.0, 80 / 127.f);
  ASSERT_TRUE(schedule);
  ASSERT_EQ(schedule->steps.size(), 7u);
  for (size_t i = 0; i < 6; ++i)
  {
    EXPECT_EQ(schedule->steps[i].pitches,
              std::vector<int>{CMajor(i % 2 ? 3 : 2)})
        << i;
    EXPECT_EQ(schedule->steps[i].duration, std::chrono::milliseconds{200}) << i;
  }
  // The last held, until the next strike.
  EXPECT_EQ(schedule->steps.back().duration.count(), 0);
  EXPECT_FLOAT_EQ(schedule->steps[0].gain, 1.f);
  EXPECT_FLOAT_EQ(schedule->steps[1].gain, 60.f / 80.f);
  EXPECT_EQ(schedule->cycleBegin, schedule->cycleEnd);
  // Struck after it began, as when the event's time is later: the notes
  // keep their times from the strike, what came before cut short.
  const auto late = PerformedOrnament(
      {trill.notes.begin(), trill.notes.begin() + 7}, 1250.0, 1.0, 80 / 127.f);
  ASSERT_TRUE(late);
  ASSERT_EQ(late->steps.size(), 5u);
  EXPECT_EQ(late->steps[0].pitches, std::vector<int>{CMajor(2)});
  EXPECT_EQ(late->steps[0].duration, std::chrono::milliseconds{50});
  EXPECT_EQ(late->steps[1].duration, std::chrono::milliseconds{100});
  // Notes struck together are one step; one step is no ornament.
  const auto together = PerformedOrnament(
      {{0.0, 100.0, 60, 50}, {10.0, 100.0, 64, 70}, {200.0, 300.0, 62, 60}},
      0.0, 1.0, 70 / 127.f);
  ASSERT_TRUE(together);
  ASSERT_EQ(together->steps.size(), 2u);
  EXPECT_EQ(together->steps[0].pitches, (std::vector<int>{60, 64}));
  EXPECT_FALSE(PerformedOrnament({{0.0, 100.0, 60, 50}}, 0.0, 1.0, 0.5f));
}
} // namespace
} // namespace dgk
