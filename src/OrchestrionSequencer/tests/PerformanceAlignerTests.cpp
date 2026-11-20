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

#include "internal/PerformanceAligner.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace dgk
{
namespace
{
constexpr int quarter = 480;
constexpr int half = 2 * quarter;
// 120 quarters per minute.
constexpr double nominalTicksPerMs = 2.0 * quarter / 1000.0;
constexpr auto none = PerformanceAlignment::none;

/** A C major pitch, `degree` steps above middle C. */
int CMajor(int degree)
{
  static constexpr int steps[] = {0, 2, 4, 5, 7, 9, 11};
  return 60 + 12 * (degree / 7) + steps[degree % 7];
}

/**
 * A scale up and down in quarters over a bass on each bar's first beat, and
 * its performance: the tempo drifting from 120 to 90 and back up to 140 per
 * minute, the bass a little after the melody.
 */
struct ScaleOverBass
{
  std::vector<ScoreEvent> events;
  std::vector<PerformedNote> notes;
  /** For each event, the performed note of each of its pitches. */
  std::vector<std::vector<size_t>> expected;

  ScaleOverBass()
  {
    double ms = 1000.0;
    for (int i = 0; i < 16; ++i)
    {
      const int tick = i * quarter;
      const int melody = CMajor(i < 8 ? i : 15 - i);
      ScoreEvent event{tick, {melody}, tick};
      std::vector<size_t> played{notes.size()};
      notes.push_back({ms, ms + 400.0, melody, 70});
      if (i % 4 == 0)
      {
        event.pitches.insert(event.pitches.begin(), 48);
        played.insert(played.begin(), notes.size());
        notes.push_back({ms + 25.0, ms + 1500.0, 48, 50});
      }
      events.push_back(event);
      expected.push_back(played);
      const double bpm =
          i < 8 ? 120.0 - 30.0 * i / 7 : 90.0 + 50.0 * (i - 8) / 7;
      ms += 60000.0 / bpm;
    }
  }
};

TEST(PerformanceAlignerTests, FollowsRubato)
{
  const ScaleOverBass piece;
  const auto alignment =
      AlignPerformance(piece.events, piece.notes, nominalTicksPerMs);
  for (size_t e = 0; e < piece.events.size(); ++e)
  {
    EXPECT_EQ(alignment.events[e].notes, piece.expected[e]) << e;
    ASSERT_TRUE(alignment.events[e].ms.has_value()) << e;
  }
  EXPECT_TRUE(alignment.added.empty());
  EXPECT_DOUBLE_EQ(*alignment.events[0].ms, 1000.0);
}

TEST(PerformanceAlignerTests, LeavesMissedNotesOutAndWrongOnesAside)
{
  ScaleOverBass piece;
  // The melody's 7th note (event 6) missed; a C sharp slipped in after the
  // 10th (event 9).
  const size_t missed = piece.expected[6][0];
  const double slipMs = piece.notes[piece.expected[9][0]].onsetMs + 150.0;
  std::vector<PerformedNote> notes;
  std::vector<size_t> renumbered(piece.notes.size(), none);
  for (size_t j = 0; j < piece.notes.size(); ++j)
  {
    if (j == missed)
      continue;
    if (piece.notes[j].onsetMs > slipMs &&
        std::none_of(notes.begin(), notes.end(),
                     [](const PerformedNote &n) { return n.pitch == 61; }))
      notes.push_back({slipMs, slipMs + 100.0, 61, 40});
    renumbered[j] = notes.size();
    notes.push_back(piece.notes[j]);
  }
  const size_t slip =
      std::find_if(notes.begin(), notes.end(),
                   [](const PerformedNote &n) { return n.pitch == 61; }) -
      notes.begin();

  const auto alignment =
      AlignPerformance(piece.events, notes, nominalTicksPerMs);
  for (size_t e = 0; e < piece.events.size(); ++e)
  {
    std::vector<size_t> expected;
    for (const size_t j : piece.expected[e])
      expected.push_back(renumbered[j]);
    EXPECT_EQ(alignment.events[e].notes, expected) << e;
  }
  EXPECT_FALSE(alignment.events[6].ms.has_value());
  EXPECT_EQ(alignment.added, std::vector<size_t>{slip});
}

TEST(PerformanceAlignerTests, GivesATrillAllItsNotes)
{
  // A half-note trill on C, then E.
  std::vector<ScoreEvent> events{{0, {72, 74}, 0, {72, 74}, half},
                                 {half, {76}, half}};
  std::vector<PerformedNote> notes;
  for (int i = 0; i < 10; ++i)
    notes.push_back({90.0 * i, 90.0 * i + 80.0, i % 2 ? 74 : 72, 60});
  notes.push_back({1000.0, 1500.0, 76, 70});

  const auto alignment = AlignPerformance(events, notes, nominalTicksPerMs);
  EXPECT_EQ(alignment.events[0].notes, (std::vector<size_t>{0, 1}));
  EXPECT_EQ(alignment.events[0].trillNotes,
            (std::vector<size_t>{2, 3, 4, 5, 6, 7, 8, 9}));
  EXPECT_EQ(alignment.events[1].notes, std::vector<size_t>{10});
  EXPECT_TRUE(alignment.added.empty());
}

TEST(PerformanceAlignerTests, JumpsOverASkippedRepeat)
{
  // |: C D E F G A B C :| D C B A G F E D, the repeat not played.
  std::vector<ScoreEvent> events;
  for (int pass = 0; pass < 2; ++pass)
    for (int i = 0; i < 8; ++i)
      events.push_back({(8 * pass + i) * quarter, {CMajor(i)}, i * quarter});
  for (int i = 0; i < 8; ++i)
    events.push_back({(16 + i) * quarter, {CMajor(8 - i)}, (8 + i) * quarter});
  std::vector<PerformedNote> notes;
  for (int i = 0; i < 16; ++i)
    notes.push_back({500.0 * i, 500.0 * i + 400.0,
                     events[i < 8 ? i : i + 8].pitches[0], 60});

  const auto alignment = AlignPerformance(events, notes, nominalTicksPerMs);
  for (size_t e = 0; e < 8; ++e)
  {
    EXPECT_EQ(alignment.events[e].notes, std::vector<size_t>{e}) << e;
    EXPECT_EQ(alignment.events[e + 8].notes, std::vector<size_t>{none}) << e;
    EXPECT_EQ(alignment.events[e + 16].notes, std::vector<size_t>{e + 8}) << e;
  }
  EXPECT_TRUE(alignment.added.empty());
}

std::string DataPath(const std::string &name)
{
  return std::string(orchestrionsequencer_tests_DATA_ROOT) + "/data/" + name;
}

/** Reads lines, skipping comments, into `parse`. */
template <typename F> void ReadLines(const std::string &path, F parse)
{
  std::ifstream file(path);
  ASSERT_TRUE(file) << path;
  for (std::string line; std::getline(file, line);)
    if (!line.empty() && line[0] != '#')
      parse(line);
}

std::vector<int> ReadInts(const std::string &text)
{
  std::istringstream in(text);
  std::vector<int> ints;
  for (int i; in >> i;)
    ints.push_back(i);
  return ints;
}

TEST(PerformanceAlignerTests, AlignsFrankLevysNocturne)
{
  std::vector<ScoreEvent> events;
  ReadLines(DataPath("nocturne-op9-2.events.txt"),
            [&](const std::string &line)
            {
              const auto bar1 = line.find('|');
              const auto bar2 = line.find('|', bar1 + 1);
              const auto head = ReadInts(line.substr(0, bar1));
              ScoreEvent event{head[0],
                               ReadInts(line.substr(bar1 + 1, bar2 - bar1 - 1)),
                               head[1]};
              event.rolled = head[2] != 0;
              event.trillEnd = head[3];
              event.trillPitches = ReadInts(line.substr(bar2 + 1));
              events.push_back(event);
            });
  std::vector<PerformedNote> notes;
  ReadLines(DataPath("nocturne-op9-2-levy.notes.txt"),
            [&](const std::string &line)
            {
              std::istringstream in(line);
              PerformedNote note;
              in >> note.onsetMs >> note.offsetMs >> note.pitch >>
                  note.velocity;
              notes.push_back(note);
            });
  ASSERT_EQ(events.size(), 592u);
  ASSERT_EQ(notes.size(), 1307u);

  const auto alignment = AlignPerformance(events, notes, 0.528);

  size_t pitches = 0, matched = 0, timed = 0, trillNotes = 0, backwards = 0;
  std::optional<double> previous;
  for (const auto &event : alignment.events)
  {
    pitches += event.notes.size();
    matched += std::count_if(event.notes.begin(), event.notes.end(),
                             [](size_t n) { return n != none; });
    trillNotes += event.trillNotes.size();
    if (event.ms)
    {
      ++timed;
      if (previous && *event.ms < *previous)
        ++backwards;
      previous = event.ms;
    }
  }
  std::printf("matched %zu of %zu pitches, %zu of %zu events timed, "
              "%zu trill notes, %zu added, %zu back in time\n",
              matched, pitches, timed, events.size(), trillNotes,
              alignment.added.size(), backwards);
  // Every performed note is accounted for, once.
  EXPECT_EQ(matched + trillNotes + alignment.added.size(), notes.size());
  EXPECT_GE(matched, pitches * 9 / 10);
  EXPECT_GE(timed, events.size() * 97 / 100);
  EXPECT_EQ(backwards, 0u);
  // The trill of bar 7: Levy plays fifteen notes, the first two matched.
  const auto bar7 =
      std::find_if(events.begin(), events.end(),
                   [](const ScoreEvent &e) { return e.tick == 17520; });
  ASSERT_NE(bar7, events.end());
  EXPECT_GE(alignment.events[bar7 - events.begin()].trillNotes.size(), 10u);
}
} // namespace
} // namespace dgk
