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

#include "internal/RecordingSyncSession.h"

namespace dgk
{
namespace
{
constexpr int eighth = 240;
constexpr int beat = 3 * eighth; // 12/8
constexpr int bar = 4 * beat;
using Precision = RecordingSyncSession::Precision;

/**
 * Four bars of 12/8, the second and third a repeat: as played, bars 0, 1, 2,
 * 1 again, 2 again, 3.
 */
std::vector<PlayedBar> Bars()
{
  const int written[] = {0, 1, 2, 1, 2, 3};
  std::vector<PlayedBar> bars;
  for (int i = 0; i < 6; ++i)
  {
    PlayedBar b;
    b.tick = i * bar;
    b.writtenTick = written[i] * bar;
    b.length = bar;
    b.beatTicks = beat;
    b.subdivisionTicks = eighth;
    b.beats = 4;
    b.number = written[i] + 1;
    b.pass = i >= 3 && i <= 4 ? 2 : 1;
    bars.push_back(b);
  }
  return bars;
}

std::vector<int> Positions(const RecordingSyncSession &session)
{
  std::vector<int> positions;
  for (int i = 0; i < static_cast<int>(session.Sync().points.size()); ++i)
    positions.push_back(session.Position(i));
  return positions;
}

/** A first pass over bar 0, a beat a second from 1 s. */
RecordingSyncSession BeatsOfBarZero()
{
  RecordingSyncSession session{Bars(), {}};
  for (int b = 0; b < 4; ++b)
    EXPECT_TRUE(session.Tap(1.0 + b));
  return session;
}

TEST(RecordingSyncSessionTests, TapsGoThroughTheScoreInOrder)
{
  const auto session = BeatsOfBarZero();
  EXPECT_EQ(Positions(session),
            (std::vector<int>{0, beat, 2 * beat, 3 * beat}));
  EXPECT_EQ(session.Next(), bar);
  EXPECT_EQ(session.Sync().points[1].bar, 0);
  EXPECT_EQ(session.Sync().points[1].tick, beat);
}

TEST(RecordingSyncSessionTests, AnotherPassRewritesFinerOrCoarser)
{
  auto session = BeatsOfBarZero();
  // Again from bar 0, in eighths, over its first beat and a half.
  session.SetPrecision(Precision::subdivisions);
  EXPECT_EQ(session.TapFrom(0), 0.0);
  for (int e = 0; e < 5; ++e)
    session.Tap(1.0 + e * 0.3);
  EXPECT_EQ(Positions(session),
            (std::vector<int>{0, eighth, 2 * eighth, beat, beat + eighth,
                              2 * beat, 3 * beat}));
  // And in beats again: the eighths between go; those past the pass's last
  // tap are still there.
  session.SetPrecision(Precision::beats);
  session.TapFrom(0);
  session.Tap(1.0);
  session.Tap(1.9);
  EXPECT_EQ(Positions(session),
            (std::vector<int>{0, beat, beat + eighth, 2 * beat, 3 * beat}));
  session.Tap(3.0);
  EXPECT_EQ(Positions(session),
            (std::vector<int>{0, beat, 2 * beat, 3 * beat}));
  EXPECT_DOUBLE_EQ(session.Sync().points[1].seconds, 1.9);
}

TEST(RecordingSyncSessionTests, KeepsTimeInOrder)
{
  auto session = BeatsOfBarZero();
  session.TapFrom(0);
  session.Tap(1.0);
  // Later than the third beat's anchor: that one goes.
  session.Tap(3.5);
  EXPECT_EQ(Positions(session), (std::vector<int>{0, beat, 3 * beat}));
}

TEST(RecordingSyncSessionTests, ChangingPrecisionAlignsTheNextTap)
{
  RecordingSyncSession session{Bars(), {}};
  session.SetPrecision(Precision::subdivisions);
  session.Tap(1.0);
  EXPECT_EQ(session.Next(), eighth);
  session.SetPrecision(Precision::beats);
  EXPECT_EQ(session.Next(), beat);
}

TEST(RecordingSyncSessionTests, TapsOnFromWhereTheSyncLeftOff)
{
  const auto first = BeatsOfBarZero();
  RecordingSyncSession again{Bars(), first.Sync()};
  EXPECT_EQ(again.Next(), bar);
  // Not anchored from bar 1 on: no going back to tap it.
  EXPECT_FALSE(again.TapFrom(1).has_value());
  // Bar 0 is: from two seconds before its first anchor.
  EXPECT_DOUBLE_EQ(*again.TapFrom(0), 0.0);
}

TEST(RecordingSyncSessionTests, UndoesEachChange)
{
  auto session = BeatsOfBarZero();
  session.TapFrom(0);
  session.Tap(1.0);
  session.Tap(3.5);
  session.SetSeconds(1, 1.5);
  session.SetSeconds(1, 1.6); // the same change, moved on
  ASSERT_TRUE(session.Undo());
  EXPECT_DOUBLE_EQ(session.Sync().points[1].seconds, 3.5);
  ASSERT_TRUE(session.Undo());
  ASSERT_TRUE(session.Undo());
  EXPECT_EQ(Positions(session),
            (std::vector<int>{0, beat, 2 * beat, 3 * beat}));
  EXPECT_DOUBLE_EQ(session.Sync().points[2].seconds, 3.0);
}

TEST(RecordingSyncSessionTests, MovesAnchorsOnlyBetweenTheirNeighbours)
{
  auto session = BeatsOfBarZero();
  session.SetSeconds(1, 5.0);
  EXPECT_DOUBLE_EQ(session.Sync().points[1].seconds, 2.99);
  session.SetSeconds(1, 0.0);
  EXPECT_DOUBLE_EQ(session.Sync().points[1].seconds, 1.01);
}

TEST(RecordingSyncSessionTests, SelectsAndDeletes)
{
  auto session = BeatsOfBarZero();
  session.Select(1, false);
  session.ExtendSelection(1, -1);
  EXPECT_EQ(session.Selection(), (std::set<int>{1, 2}));
  session.Select(1, true); // taken from the selection
  EXPECT_EQ(session.Selection(), (std::set<int>{2}));
  session.ExtendSelection(-1, -1);
  ASSERT_TRUE(session.DeleteSelection());
  EXPECT_EQ(Positions(session), (std::vector<int>{0, 3 * beat}));
  EXPECT_TRUE(session.Selection().empty());
  // Nothing selected: from the anchor given.
  session.ExtendSelection(1, 0);
  EXPECT_EQ(session.Selection(), (std::set<int>{0}));
  EXPECT_TRUE(session.ClearSelection());
  EXPECT_FALSE(session.ClearSelection());
  ASSERT_TRUE(session.Undo());
  EXPECT_EQ(Positions(session).size(), 4u);
}

TEST(RecordingSyncSessionTests, FindsTheBarClickedInTheNearestPass)
{
  const RecordingSyncSession session{Bars(), {}};
  // Bar 2 as written is played as bars 2 and 4.
  EXPECT_EQ(session.BarAt(2 * bar + beat, 0), 2);
  EXPECT_EQ(session.BarAt(2 * bar + beat, 4 * bar), 4);
  EXPECT_FALSE(session.BarAt(10 * bar, 0).has_value());
}

TEST(RecordingSyncSessionTests, PlacesPositions)
{
  const RecordingSyncSession session{Bars(), {}};
  const auto place = session.PlaceOf(4 * bar + beat + 2 * eighth);
  EXPECT_EQ(place.bar, 4);
  EXPECT_EQ(place.beat, 2);
  EXPECT_EQ(place.subdivision, 3);
  EXPECT_EQ(place.subdivisions, 3);
}

TEST(RecordingSyncSessionTests, StopsAtTheEnd)
{
  RecordingSyncSession session{Bars(), {}};
  for (int b = 0; b < 6 * 4; ++b)
    EXPECT_TRUE(session.Tap(1.0 + b));
  EXPECT_FALSE(session.Next().has_value());
  EXPECT_FALSE(session.Tap(100.0));
}
} // namespace
} // namespace dgk
