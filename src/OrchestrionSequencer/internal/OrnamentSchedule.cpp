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
#include "OrnamentSchedule.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>

namespace dgk
{
namespace
{
// The note value of an ornament note — a trill's alternation, a turn's notes:
// a 32nd, at the performer's tempo.
constexpr int ornamentNoteTicks = 60;
// No ornament note shorter than this: the note values above are floored here
// at fast tempi, and a figure that would need shorter notes is dropped.
constexpr double minOrnamentNoteMs = 40.0;

std::chrono::microseconds Us(double ms)
{
  return std::chrono::microseconds{std::llround(ms * 1000.0)};
}

/**
 * Appends the graces at the values they carry, none shorter than the minimum,
 * all scaled down to fit `capMs` if they exceed it. Returns their total
 * length.
 */
double AppendGraces(std::vector<TimedOrnamentStep> &steps,
                    const std::vector<GraceNote> &graces, double ticksPerMs,
                    double capMs)
{
  std::vector<double> ms;
  for (const GraceNote &grace : graces)
    ms.push_back(std::max(grace.ticks / ticksPerMs, minOrnamentNoteMs));
  const double total = std::accumulate(ms.begin(), ms.end(), 0.0);
  if (total <= 0.0)
    return 0.0;
  const double scale = total > capMs ? capMs / total : 1.0;
  for (size_t i = 0; i < graces.size(); ++i)
    steps.push_back({graces[i].pitches, Us(ms[i] * scale)});
  return total * scale;
}
} // namespace

OrnamentSchedule PlainChord(std::vector<int> pitches)
{
  OrnamentSchedule schedule;
  schedule.steps.push_back({std::move(pitches), std::chrono::microseconds{0}});
  return schedule;
}

OrnamentSchedule ScheduleOrnament(const Ornament &ornament,
                                  const std::vector<int> &mainPitches,
                                  int nominalTicks, double ticksPerMs)
{
  assert(ticksPerMs > 0.0);
  OrnamentSchedule schedule;
  const double noteMs = nominalTicks / ticksPerMs;
  const double ornamentNoteMs =
      std::max(ornamentNoteTicks / ticksPerMs, minOrnamentNoteMs);

  // Grace notes take half the note at most; the closing ones sound at the
  // leaving gesture and take nothing from it.
  const double graceMs = AppendGraces(schedule.steps, ornament.gracesBefore,
                                      ticksPerMs, noteMs / 2);
  schedule.anticipation = ornament.appoggiatura ? Us(0.0) : Us(graceMs);
  const double remaining = noteMs - graceMs;
  AppendGraces(schedule.closing, ornament.gracesAfter, ticksPerMs, noteMs / 2);

  const auto hold = [&](const std::vector<int> &pitches)
  { schedule.steps.push_back({pitches, std::chrono::microseconds{0}}); };

  switch (ornament.sign)
  {
  case OrnamentSign::none:
    hold(mainPitches);
    break;
  case OrnamentSign::figure:
  {
    const auto &figure = ornament.figure;
    assert(!figure.empty());
    // At natural speed if the note has room for the figure, the main note
    // then held; else compressed to fill the note.
    double noteLength = ornamentNoteMs;
    if (remaining < figure.size() * ornamentNoteMs)
    {
      noteLength = remaining / figure.size();
      if (noteLength < minOrnamentNoteMs)
      {
        hold(mainPitches); // too short to ornament
        break;
      }
    }
    for (size_t i = 0; i + 1 < figure.size(); ++i)
      schedule.steps.push_back({figure[i], Us(noteLength)});
    hold(figure.back());
    break;
  }
  case OrnamentSign::trill:
  {
    const auto &pair = ornament.figure;
    assert(pair.size() == 2);
    if (remaining < 3 * ornamentNoteMs)
    {
      // No room for three 32nds: the short trill's figure — main, upper,
      // main — compressed to fill the note, if that leaves it notes long
      // enough.
      const double noteLength = remaining / 3.0;
      if (noteLength < minOrnamentNoteMs)
      {
        hold(mainPitches); // too short to ornament
        break;
      }
      schedule.steps.push_back({pair[0], Us(noteLength)});
      schedule.steps.push_back({pair[1], Us(noteLength)});
      hold(pair[0]);
      break;
    }
    // 32nds, kept up until the leaving gesture — which takes effect when the
    // note under way ends, so the last one is no shorter than the others.
    schedule.cycleBegin = schedule.steps.size();
    schedule.steps.push_back({pair[0], Us(ornamentNoteMs)});
    schedule.steps.push_back({pair[1], Us(ornamentNoteMs)});
    schedule.cycleEnd = schedule.steps.size();
    break;
  }
  }
  return schedule;
}

namespace
{
/**
 * The odd number of notes, three at least, that divides `spanTicks` into
 * notes closest to a 32nd — odd, so that a trill started on the main note
 * ends on it.
 */
int TrillNoteCount(int spanTicks)
{
  const double pairs = (spanTicks / double(ornamentNoteTicks) - 1.0) / 2.0;
  return 2 * std::max(1, static_cast<int>(std::lround(pairs))) + 1;
}
} // namespace

std::vector<WrittenNote> WriteOutOrnament(const Ornament &ornament,
                                          const std::vector<int> &mainPitches,
                                          int nominalTicks, double ticksPerMs)
{
  const OrnamentSchedule schedule =
      ScheduleOrnament(ornament, mainPitches, nominalTicks, ticksPerMs);
  const auto ticksOf = [ticksPerMs](const TimedOrnamentStep &step)
  {
    return static_cast<int>(
        std::lround(step.duration.count() / 1000.0 * ticksPerMs));
  };

  std::vector<WrittenNote> notes;
  // The closing notes come last, at their length, taking half the chord at
  // most from what the others share out.
  std::vector<int> closingTicks;
  int closingTotal = 0;
  for (const TimedOrnamentStep &step : schedule.closing)
  {
    closingTicks.push_back(std::max(1, ticksOf(step)));
    closingTotal += closingTicks.back();
  }
  if (closingTotal > nominalTicks / 2)
  {
    const double scale = (nominalTicks / 2) / double(closingTotal);
    closingTotal = 0;
    for (int &ticks : closingTicks)
    {
      ticks = std::max(1, static_cast<int>(std::lround(ticks * scale)));
      closingTotal += ticks;
    }
  }
  int left = nominalTicks - closingTotal;

  const bool cycling = schedule.cycleBegin < schedule.cycleEnd;
  // The timed steps — all of them, or those before the trill — at their
  // schedule's length; a held one takes what is left.
  const size_t fixed = cycling ? schedule.cycleBegin : schedule.steps.size();
  for (size_t i = 0; i < fixed; ++i)
  {
    const TimedOrnamentStep &step = schedule.steps[i];
    const bool held = step.duration.count() == 0;
    const int ticks = held ? std::max(left, 1)
                           : std::clamp(ticksOf(step), 1, std::max(left, 1));
    notes.push_back({step.pitches, ticks});
    left -= ticks;
  }
  if (cycling)
  {
    // The trill: the odd number of 32nds that divides what is left.
    const int count = TrillNoteCount(left);
    const size_t length = schedule.cycleEnd - schedule.cycleBegin;
    for (int i = 0; i < count; ++i)
    {
      const int ticks = std::max(1, left / (count - i));
      notes.push_back(
          {schedule.steps[schedule.cycleBegin + i % length].pitches, ticks});
      left -= ticks;
    }
  }
  for (size_t i = 0; i < schedule.closing.size(); ++i)
    notes.push_back({schedule.closing[i].pitches, closingTicks[i]});
  return notes;
}
} // namespace dgk
