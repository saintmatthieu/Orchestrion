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

#include "internal/OrnamentSchedule.h"

#include <vector>

namespace dgk
{
namespace
{
constexpr int ticksPerQuarter = 480;
// Ticks per millisecond at the given quarter-notes-per-minute.
constexpr double TicksPerMs(double bpm)
{
  return bpm * ticksPerQuarter / 60000.0;
}

constexpr int whole = 4 * ticksPerQuarter;
constexpr int half = 2 * ticksPerQuarter;
constexpr int quarter = ticksPerQuarter;
constexpr int eighth = ticksPerQuarter / 2;
constexpr int sixteenth = ticksPerQuarter / 4;
constexpr int thirtySecond = ticksPerQuarter / 8;

const std::vector<int> C{72};
const std::vector<int> D{74};
const std::vector<int> B{71};

Ornament Trill()
{
  Ornament ornament;
  ornament.sign = OrnamentSign::trill;
  ornament.figure = {C, D};
  return ornament;
}

Ornament Turn()
{
  Ornament ornament;
  ornament.sign = OrnamentSign::figure;
  ornament.figure = {C, D, C, B, C};
  return ornament;
}

double Ms(const TimedOrnamentStep &step)
{
  return step.duration.count() / 1000.0;
}

bool Holds(const TimedOrnamentStep &step) { return step.duration.count() == 0; }
} // namespace

TEST(OrnamentScheduleTests, PlainChordIsOneHeldStep)
{
  const OrnamentSchedule s = PlainChord(C);
  ASSERT_EQ(s.steps.size(), 1u);
  EXPECT_EQ(s.steps[0].pitches, C);
  EXPECT_TRUE(Holds(s.steps[0]));
  EXPECT_EQ(s.cycleBegin, s.cycleEnd);
}

// A trill is 32nds — 125 ms at 60 — cycling C D.
TEST(OrnamentScheduleTests, TrillIs32nds)
{
  const OrnamentSchedule s =
      ScheduleOrnament(Trill(), C, eighth, TicksPerMs(60));
  ASSERT_EQ(s.steps.size(), 2u);
  EXPECT_EQ(s.cycleBegin, 0u);
  EXPECT_EQ(s.cycleEnd, 2u);
  EXPECT_EQ(s.steps[0].pitches, C);
  EXPECT_EQ(s.steps[1].pitches, D);
  EXPECT_NEAR(Ms(s.steps[0]), 125.0, 0.01);
  EXPECT_NEAR(Ms(s.steps[1]), 125.0, 0.01);
}

// A semiquaver has no room for three 32nds: the short trill's figure C D C,
// compressed to 250 / 3 ms each, the last held.
TEST(OrnamentScheduleTests, TrillOnASemiquaverIsAShortTrillFigure)
{
  const OrnamentSchedule s =
      ScheduleOrnament(Trill(), C, sixteenth, TicksPerMs(60));
  ASSERT_EQ(s.steps.size(), 3u);
  EXPECT_EQ(s.cycleBegin, s.cycleEnd);
  EXPECT_EQ(s.steps[0].pitches, C);
  EXPECT_EQ(s.steps[1].pitches, D);
  EXPECT_EQ(s.steps[2].pitches, C);
  EXPECT_NEAR(Ms(s.steps[0]), 250.0 / 3.0, 0.01);
  EXPECT_NEAR(Ms(s.steps[1]), 250.0 / 3.0, 0.01);
  EXPECT_TRUE(Holds(s.steps[2]));
}

// The same 32nds whatever the note's length.
TEST(OrnamentScheduleTests, TrillSpeedDoesNotDependOnTheNoteLength)
{
  const OrnamentSchedule s = ScheduleOrnament(Trill(), C, half, TicksPerMs(60));
  EXPECT_NEAR(Ms(s.steps[0]), 125.0, 0.01);
}

// Ornament notes are note values: half the tempo, twice the length.
TEST(OrnamentScheduleTests, TrillSpeedFollowsTheTempo)
{
  const OrnamentSchedule slow =
      ScheduleOrnament(Trill(), C, eighth, TicksPerMs(30));
  const OrnamentSchedule fast =
      ScheduleOrnament(Trill(), C, eighth, TicksPerMs(120));
  EXPECT_NEAR(Ms(slow.steps[0]), 250.0, 0.01);
  EXPECT_NEAR(Ms(fast.steps[0]), 62.5, 0.01);
}

// At a very fast tempo the 32nd (31 ms at 240) is floored at 40 ms.
TEST(OrnamentScheduleTests, TrillNoteLengthIsFlooredAtFastTempi)
{
  const OrnamentSchedule s =
      ScheduleOrnament(Trill(), C, quarter, TicksPerMs(240));
  EXPECT_NEAR(Ms(s.steps[0]), 40.0, 0.01);
}

// Too short to trill: a demisemiquaver at 90 (83 ms) can't hold three notes
// of 40 ms.
TEST(OrnamentScheduleTests, TrillOnTooShortANoteIsPlain)
{
  const OrnamentSchedule s =
      ScheduleOrnament(Trill(), C, thirtySecond, TicksPerMs(90));
  ASSERT_EQ(s.steps.size(), 1u);
  EXPECT_EQ(s.steps[0].pitches, C);
  EXPECT_TRUE(Holds(s.steps[0]));
  EXPECT_EQ(s.cycleBegin, s.cycleEnd);
}

// Chopin Op. 9/2 bar 2: a turn on a quaver has no room for five 32nds, so the
// five notes divide the quaver evenly, the last running into the next note.
TEST(OrnamentScheduleTests, TurnOnAQuaverIsCompressedToFillIt)
{
  const OrnamentSchedule s =
      ScheduleOrnament(Turn(), C, eighth, TicksPerMs(60));
  ASSERT_EQ(s.steps.size(), 5u);
  const std::vector<std::vector<int>> expected{C, D, C, B, C};
  for (size_t i = 0; i < 4; ++i)
  {
    EXPECT_EQ(s.steps[i].pitches, expected[i]);
    EXPECT_NEAR(Ms(s.steps[i]), 100.0, 0.01);
  }
  EXPECT_EQ(s.steps[4].pitches, C);
  EXPECT_TRUE(Holds(s.steps[4]));
  EXPECT_EQ(s.cycleBegin, s.cycleEnd);
}

// A crotchet has room: 32nds of 125 ms, then the main note held.
TEST(OrnamentScheduleTests, TurnOnACrotchetIsAtNaturalSpeedThenHeld)
{
  const OrnamentSchedule s =
      ScheduleOrnament(Turn(), C, quarter, TicksPerMs(60));
  ASSERT_EQ(s.steps.size(), 5u);
  for (size_t i = 0; i < 4; ++i)
    EXPECT_NEAR(Ms(s.steps[i]), 125.0, 0.01);
  EXPECT_TRUE(Holds(s.steps[4]));
}

// Five notes in a demisemiquaver (125 ms) would be 25 ms each: plain.
TEST(OrnamentScheduleTests, TurnOnTooShortANoteIsPlain)
{
  const OrnamentSchedule s =
      ScheduleOrnament(Turn(), C, thirtySecond, TicksPerMs(60));
  ASSERT_EQ(s.steps.size(), 1u);
  EXPECT_EQ(s.steps[0].pitches, C);
  EXPECT_TRUE(Holds(s.steps[0]));
}

// Graces play at the value they carry: 64ths here, 62.5 ms at 60.
TEST(OrnamentScheduleTests, GracesPlayAtTheValueTheyCarry)
{
  Ornament ornament;
  ornament.gracesBefore = {{{67}, thirtySecond / 2},
                           {{70}, thirtySecond / 2},
                           {{75}, thirtySecond / 2}};
  const OrnamentSchedule s =
      ScheduleOrnament(ornament, {79}, quarter, TicksPerMs(60));
  ASSERT_EQ(s.steps.size(), 4u);
  for (size_t i = 0; i < 3; ++i)
    EXPECT_NEAR(Ms(s.steps[i]), 62.5, 0.01);
  EXPECT_EQ(s.steps[3].pitches, std::vector<int>{79});
  EXPECT_TRUE(Holds(s.steps[3]));
}

// A lone unslashed grace is an appoggiatura and takes its written value ...
TEST(OrnamentScheduleTests, AppoggiaturaTakesItsWrittenValue)
{
  Ornament ornament;
  ornament.gracesBefore = {{D, eighth}};
  const OrnamentSchedule s =
      ScheduleOrnament(ornament, C, half, TicksPerMs(60));
  ASSERT_EQ(s.steps.size(), 2u);
  EXPECT_NEAR(Ms(s.steps[0]), 500.0, 0.01);
  EXPECT_TRUE(Holds(s.steps[1]));
}

// ... capped at half the chord.
TEST(OrnamentScheduleTests, GracesTakeHalfTheChordAtMost)
{
  Ornament ornament;
  ornament.gracesBefore = {{D, half}};
  const OrnamentSchedule s =
      ScheduleOrnament(ornament, C, quarter, TicksPerMs(60));
  EXPECT_NEAR(Ms(s.steps[0]), 500.0, 0.01);
}

// Graces after the chord are its closing steps.
TEST(OrnamentScheduleTests, GracesAfterAreClosingSteps)
{
  Ornament ornament;
  ornament.gracesAfter = {{D, thirtySecond}, {{76}, thirtySecond}};
  const OrnamentSchedule s =
      ScheduleOrnament(ornament, C, half, TicksPerMs(60));
  ASSERT_EQ(s.steps.size(), 1u);
  EXPECT_TRUE(Holds(s.steps[0]));
  ASSERT_EQ(s.closing.size(), 2u);
  EXPECT_NEAR(Ms(s.closing[0]), 125.0, 0.01);
  EXPECT_NEAR(Ms(s.closing[1]), 125.0, 0.01);
}

// No grace shorter than the minimum: a 64th at 240 (16 ms) is stretched to
// 40 ms.
TEST(OrnamentScheduleTests, GracesAreFlooredAtFastTempi)
{
  Ornament ornament;
  ornament.gracesBefore = {{B, thirtySecond / 2}};
  const OrnamentSchedule s =
      ScheduleOrnament(ornament, C, quarter, TicksPerMs(240));
  EXPECT_NEAR(Ms(s.steps[0]), 40.0, 0.01);
}

// A trill after graces cycles the two steps that follow them.
TEST(OrnamentScheduleTests, TrillAfterGracesCyclesPastThem)
{
  Ornament ornament = Trill();
  ornament.gracesBefore = {{B, thirtySecond}};
  const OrnamentSchedule s =
      ScheduleOrnament(ornament, C, half, TicksPerMs(60));
  ASSERT_EQ(s.steps.size(), 3u);
  EXPECT_EQ(s.steps[0].pitches, B);
  EXPECT_EQ(s.cycleBegin, 1u);
  EXPECT_EQ(s.cycleEnd, 3u);
}

// ---- The manual mode: the ornament written out as gestures.

int TotalTicks(const std::vector<WrittenNote> &notes)
{
  int total = 0;
  for (const WrittenNote &note : notes)
    total += note.ticks;
  return total;
}

// A trill on a quaver: five 32nds of 48 ticks, C D C D C.
TEST(WriteOutOrnamentTests, TrillOnAQuaverIsFiveNotes)
{
  const auto notes = WriteOutOrnament(Trill(), C, eighth, TicksPerMs(60));
  ASSERT_EQ(notes.size(), 5u);
  const std::vector<std::vector<int>> expected{C, D, C, D, C};
  for (size_t i = 0; i < 5; ++i)
  {
    EXPECT_EQ(notes[i].pitches, expected[i]);
    EXPECT_EQ(notes[i].ticks, 48);
  }
}

// A semiquaver: three notes of 40 ticks, whatever the tempo — as long as the
// notes stay playable (at 160 they would be 31 ms: the chord plays plain).
TEST(WriteOutOrnamentTests, TrillOnASemiquaverIsThreeNotesInTicks)
{
  for (const double bpm : {40.0, 60.0, 90.0})
  {
    const auto notes = WriteOutOrnament(Trill(), C, sixteenth, TicksPerMs(bpm));
    ASSERT_EQ(notes.size(), 3u);
    EXPECT_EQ(TotalTicks(notes), sixteenth);
  }
}

// A minim: 960 / 60 = 16 → 17 notes sharing 960 ticks.
TEST(WriteOutOrnamentTests, TrillOnAMinimRoundsToAnOddCount)
{
  const auto notes = WriteOutOrnament(Trill(), C, half, TicksPerMs(60));
  EXPECT_EQ(notes.size(), 17u);
  EXPECT_EQ(TotalTicks(notes), half);
  EXPECT_EQ(notes.back().pitches, C);
}

// A turn on a quaver: compressed, five notes of 48 ticks.
TEST(WriteOutOrnamentTests, TurnOnAQuaverIsSharedEvenly)
{
  const auto notes = WriteOutOrnament(Turn(), C, eighth, TicksPerMs(60));
  ASSERT_EQ(notes.size(), 5u);
  for (const WrittenNote &note : notes)
    EXPECT_EQ(note.ticks, 48);
}

// A turn on a crotchet: four 32nds, the main note taking the rest.
TEST(WriteOutOrnamentTests, TurnOnACrotchetLeavesTheRestToTheMainNote)
{
  const auto notes = WriteOutOrnament(Turn(), C, quarter, TicksPerMs(60));
  ASSERT_EQ(notes.size(), 5u);
  for (size_t i = 0; i < 4; ++i)
    EXPECT_EQ(notes[i].ticks, thirtySecond);
  EXPECT_EQ(notes[4].ticks, quarter - 4 * thirtySecond);
  EXPECT_EQ(notes[4].pitches, C);
}

// Graces then a trill: the grace at its value, the trill dividing the rest.
TEST(WriteOutOrnamentTests, GracesComeFirstThenTheTrillDividesTheRest)
{
  Ornament ornament = Trill();
  ornament.gracesBefore = {{B, thirtySecond}};
  const auto notes = WriteOutOrnament(ornament, C, half, TicksPerMs(60));
  ASSERT_EQ(notes.size(), 16u); // the grace, then 900 / 60 = 15 notes
  EXPECT_EQ(notes[0].pitches, B);
  EXPECT_EQ(notes[0].ticks, thirtySecond);
  EXPECT_EQ(TotalTicks(notes), half);
}

// Graces after come last, taken from the main note's end.
TEST(WriteOutOrnamentTests, GracesAfterComeLast)
{
  Ornament ornament;
  ornament.gracesAfter = {{D, thirtySecond / 2}, {{76}, thirtySecond / 2}};
  const auto notes = WriteOutOrnament(ornament, C, half, TicksPerMs(60));
  ASSERT_EQ(notes.size(), 3u);
  EXPECT_EQ(notes[0].pitches, C);
  EXPECT_EQ(notes[0].ticks, half - thirtySecond);
  EXPECT_EQ(notes[1].pitches, D);
  EXPECT_EQ(notes[1].ticks, thirtySecond / 2);
  EXPECT_EQ(TotalTicks(notes), half);
}

// Too short to ornament: one note, the chord itself.
TEST(WriteOutOrnamentTests, TooShortIsOneNote)
{
  const auto notes = WriteOutOrnament(Turn(), C, thirtySecond, TicksPerMs(60));
  ASSERT_EQ(notes.size(), 1u);
  EXPECT_EQ(notes[0].pitches, C);
  EXPECT_EQ(notes[0].ticks, thirtySecond);
}
} // namespace dgk
