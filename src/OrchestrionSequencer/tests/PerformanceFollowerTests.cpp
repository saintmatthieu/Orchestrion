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

#include "internal/PerformanceFollower.h"
#include "internal/ScoreEvents.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <random>
#include <set>
#include <vector>

namespace dgk
{
namespace
{
constexpr int quarter = 480;
constexpr int eighth = quarter / 2;
constexpr int sixteenth = quarter / 4;
constexpr int bar = 4 * quarter;
// 120 quarters per minute.
constexpr double nominalTicksPerMs = 2.0 * quarter / 1000.0;

/**
 * mt19937's output is specified, unlike the standard distributions': the
 * same takes on every platform.
 */
class Random
{
public:
  explicit Random(uint32_t seed) : m_engine{seed} {}
  /** In [0, 1). */
  double Uniform() { return m_engine() / 4294967296.0; }
  int Int(int from, int to) // inclusive
  {
    return from + static_cast<int>(Uniform() * (to - from + 1));
  }
  bool Chance(double p) { return Uniform() < p; }
  double Normal()
  {
    // Box-Muller.
    const auto u = 1.0 - Uniform();
    return std::sqrt(-2.0 * std::log(u)) * std::cos(2.0 * M_PI * Uniform());
  }

private:
  std::mt19937 m_engine;
};

/** A C major pitch, `degree` steps above middle C's octave's C. */
int CMajor(int degree)
{
  static constexpr int steps[] = {0, 2, 4, 5, 7, 9, 11};
  const auto octave = (degree >= 0 ? degree : degree - 6) / 7;
  return 60 + 12 * octave + steps[degree - 7 * octave];
}

/** Notes by tick, merged into events. */
std::vector<ScoreEvent> ToEvents(const std::map<int, std::set<int>> &notes)
{
  std::vector<ScoreEvent> events;
  for (const auto &[tick, pitches] : notes)
    events.push_back({tick, {pitches.begin(), pitches.end()}, tick});
  return events;
}

/**
 * Eight bars for two hands: a right-hand line, mostly eighths, some runs of
 * sixteenths and some held notes, over left-hand triads on beats 1 and 3.
 */
void AddSection(std::map<int, std::set<int>> &notes, int startTick,
                uint32_t seed)
{
  Random random{seed};
  static constexpr int roots[] = {0, 3, 4, 5, 1, 4, 0, 0}; // I IV V vi ii V I
  auto degree = 9;
  for (auto b = 0; b < 8; ++b)
  {
    const auto barTick = startTick + b * bar;
    for (const auto beat : {0, 2})
      for (const auto third : {0, 2, 4})
        notes[barTick + beat * quarter].insert(CMajor(roots[b] + third - 7));
    auto tick = 0;
    while (tick < bar)
    {
      const auto r = random.Uniform();
      const auto length = r < 0.15 && tick % quarter == 0 ? quarter
                          : r < 0.35                      ? sixteenth
                                                          : eighth;
      notes[barTick + tick].insert(CMajor(degree));
      degree = std::clamp(degree + random.Int(-2, 2), 4, 16);
      tick += length;
    }
  }
}

/** A 32-bar piece, A A B A: the repeats make it self-similar. */
std::vector<ScoreEvent> Piece()
{
  std::map<int, std::set<int>> notes;
  AddSection(notes, 0, 1);
  AddSection(notes, 8 * bar, 1);
  AddSection(notes, 16 * bar, 2);
  AddSection(notes, 24 * bar, 1);
  return ToEvents(notes);
}

/** The index of the event bar `b` starts with. */
size_t BarStart(const std::vector<ScoreEvent> &events, int b)
{
  return static_cast<size_t>(std::find_if(events.begin(), events.end(),
                                          [&](const ScoreEvent &e)
                                          { return e.tick == b * bar; }) -
                             events.begin());
}

struct PlayedNote
{
  int pitch = 0;
  double ms = 0.0;
  /** The event the note is played for; -1 for an extra note. */
  int truth = -1;
  /** The performer's tempo then. */
  double ticksPerMs = 0.0;
};

/** A restart: when `atEvent` is reached, go back to `toEvent`. */
struct Restart
{
  size_t atEvent = 0;
  size_t toEvent = 0;
};

struct PerformanceStyle
{
  /** The performer's tempo, relative to the score's. */
  double tempo = 1.0;
  /** Standard deviation of the log-tempo's change per event. */
  double drift = 0.0;
  double miss = 0.0;
  double wrong = 0.0;
  /** Chance of an extra note, per event. */
  double extra = 0.0;
  /** How far apart a chord's notes are played, at most. */
  double chordSpreadMs = 0.0;
  /** How long the performer pauses before restarting. */
  double restartPauseMs = 1000.0;
  /**
   * Chance, per event, that the performer stops to think before it: the time
   * to it is then 2 to `hesitationMaxFactor` times as long.
   */
  double hesitate = 0.0;
  double hesitationMaxFactor = 10.0;
  std::vector<Restart> restarts;
};

std::vector<PlayedNote> Perform(const std::vector<ScoreEvent> &events,
                                const PerformanceStyle &style, uint32_t seed)
{
  Random random{seed};
  std::vector<PlayedNote> take;
  auto ticksPerMs = nominalTicksPerMs * style.tempo;
  auto ms = 0.0;
  auto restarts = style.restarts;
  for (size_t e = 0; e < events.size(); ++e)
  {
    if (!restarts.empty() && restarts.front().atEvent == e)
    {
      e = restarts.front().toEvent;
      restarts.erase(restarts.begin());
      ms += style.restartPauseMs;
    }
    else if (e > 0)
    {
      auto dt = (events[e].tick - events[e - 1].tick) / ticksPerMs;
      // Drawn only when hesitating at all: the other takes stay the same.
      if (style.hesitate > 0 && random.Chance(style.hesitate))
        dt *= 2.0 + random.Uniform() * (style.hesitationMaxFactor - 2.0);
      ms += dt;
    }
    ticksPerMs *= std::exp(style.drift * random.Normal());

    for (const auto pitch : events[e].pitches)
    {
      if (random.Chance(style.miss))
        continue;
      const auto played =
          random.Chance(style.wrong)
              ? pitch + (random.Chance(0.5) ? 1 : -1) * random.Int(1, 2)
              : pitch;
      take.push_back({played, ms + random.Uniform() * style.chordSpreadMs,
                      static_cast<int>(e), ticksPerMs});
    }
    if (random.Chance(style.extra))
      take.push_back({events[e].pitches.front() + random.Int(-5, 5),
                      ms + 30.0 + random.Uniform() * 60.0, -1, ticksPerMs});
  }
  std::stable_sort(take.begin(), take.end(),
                   [](const PlayedNote &a, const PlayedNote &b)
                   { return a.ms < b.ms; });
  return take;
}

struct Result
{
  /** Of the notes of the score played (not the extra ones). */
  double accuracy = 0.0;
  /** The same for FollowedEvent(). */
  double followedAccuracy = 0.0;
  double finalTempoRatio = 0.0;
};

Result Follow(const std::vector<ScoreEvent> &events,
              const std::vector<PlayedNote> &take,
              std::vector<PerformanceFollower::Estimate> *estimates = nullptr)
{
  PerformanceFollower follower{events, nominalTicksPerMs};
  auto right = 0;
  auto followedRight = 0;
  auto count = 0;
  for (const auto &note : take)
  {
    const auto estimate = follower.OnNoteOn(note.pitch, note.ms);
    if (estimates)
      estimates->push_back(estimate);
    if (note.truth < 0)
      continue;
    ++count;
    right += estimate.event == static_cast<size_t>(note.truth);
    followedRight +=
        follower.FollowedEvent() == static_cast<size_t>(note.truth);
  }
  return {static_cast<double>(right) / count,
          static_cast<double>(followedRight) / count,
          follower.TicksPerMs() / take.back().ticksPerMs};
}

Result Follow(const PerformanceStyle &style, uint32_t seed)
{
  const auto events = Piece();
  return Follow(events, Perform(events, style, seed));
}

/** Averaged over several takes. */
Result FollowTakes(const PerformanceStyle &style, int takes = 10)
{
  Result sum;
  for (auto seed = 1; seed <= takes; ++seed)
  {
    const auto r = Follow(style, 100 + seed);
    sum.accuracy += r.accuracy / takes;
    sum.followedAccuracy += r.followedAccuracy / takes;
    sum.finalTempoRatio += r.finalTempoRatio / takes;
  }
  return sum;
}
} // namespace

TEST(PerformanceFollowerTests, PerfectPerformanceIsFollowedExactly)
{
  const auto r = Follow(PerformanceStyle{}, 1);
  EXPECT_EQ(r.accuracy, 1.0);
  EXPECT_EQ(r.followedAccuracy, 1.0);
}

TEST(PerformanceFollowerTests, ChordsPlayedUnevenlyAreFollowed)
{
  PerformanceStyle style;
  style.chordSpreadMs = 40.0;
  EXPECT_GE(FollowTakes(style).accuracy, 0.99);
}

TEST(PerformanceFollowerTests, TempoIsLearnt)
{
  for (const auto tempo : {0.6, 0.8, 1.3, 1.6})
  {
    PerformanceStyle style;
    style.tempo = tempo;
    style.chordSpreadMs = 20.0;
    const auto r = FollowTakes(style);
    EXPECT_GE(r.accuracy, 0.98) << "tempo " << tempo;
    EXPECT_NEAR(r.finalTempoRatio, 1.0, 0.1) << "tempo " << tempo;
  }
}

TEST(PerformanceFollowerTests, DriftingTempoIsFollowed)
{
  PerformanceStyle style;
  style.drift = 0.03;
  style.chordSpreadMs = 20.0;
  EXPECT_GE(FollowTakes(style).accuracy, 0.97);
}

TEST(PerformanceFollowerTests, HesitationsAreFollowed)
{
  PerformanceStyle style;
  style.chordSpreadMs = 20.0;
  style.hesitate = 0.1;
  const auto r = FollowTakes(style);
  EXPECT_GE(r.accuracy, 0.98);
  EXPECT_GE(r.followedAccuracy, 0.98);
}

TEST(PerformanceFollowerTests, SlowLongNotesAreFollowed)
{
  // Whole notes, played at 0.6 of the score's tempo: 3.3 s apart. (At half
  // of it, the follower's own estimate would never catch up: the app gives it
  // the sequencer's.)
  std::map<int, std::set<int>> notes;
  Random random{7};
  for (auto b = 0; b < 32; ++b)
    notes[b * bar].insert(CMajor(random.Int(0, 7)));
  const auto events = ToEvents(notes);
  PerformanceStyle style;
  style.tempo = 0.6;
  style.wrong = 0.1;
  auto right = 0.0;
  for (auto seed = 1; seed <= 10; ++seed)
    right += Follow(events, Perform(events, style, 100 + seed))
                 .followedAccuracy /
             10;
  // What it misses are wrong notes that are the note before, or the next:
  // nothing tells them from the right one.
  EXPECT_GE(right, 0.97);
}

TEST(PerformanceFollowerTests, PlayingOnAfterAStruggleIsFollowed)
{
  // A run of 32nds the learner can only manage slowly, then quarters played
  // at the score's tempo: the tempo learnt from the run makes them sound
  // early. The second quarter is a semitone below the first, like a wrong
  // note for it would be.
  constexpr int thirtySecond = quarter / 8;
  std::vector<ScoreEvent> events;
  for (auto i = 0; i < 16; ++i)
    events.push_back({i * thirtySecond, {CMajor(i % 8)}, i * thirtySecond});
  const auto quarters = 16 * thirtySecond;
  const int pitches[] = {75, 74, 72, 70, 72};
  for (auto i = 0; i < 5; ++i)
    events.push_back(
        {quarters + i * quarter, {pitches[i]}, quarters + i * quarter});

  // The tempo the sequencer gives the follower, learnt from the run and not
  // caught up yet.
  constexpr double runMs = 400.0;
  PerformanceFollower follower{events, nominalTicksPerMs};
  auto ms = 0.0;
  for (size_t e = 0; e < events.size(); ++e)
  {
    if (e > 0)
      follower.SetTicksPerMs(thirtySecond / runMs);
    follower.OnNoteOn(events[e].pitches.front(), ms);
    EXPECT_EQ(follower.FollowedEvent(), e) << "event " << e;
    ms += e < 16 ? runMs : quarter / nominalTicksPerMs;
  }
}

TEST(PerformanceFollowerTests, RushedSingleNotesAreNotTakenForChords)
{
  // Single notes a 16th apart, stepwise, the learner playing them in rushed
  // pairs: the second of each pair 10 ms after the first. A single note has
  // no other note to wait for.
  constexpr int sixteenthTicks = quarter / 4;
  std::vector<ScoreEvent> events;
  for (auto i = 0; i < 16; ++i)
    events.push_back({i * sixteenthTicks, {CMajor(i)}, i * sixteenthTicks});
  PerformanceFollower follower{events, nominalTicksPerMs};
  const auto pairMs = 2 * sixteenthTicks / nominalTicksPerMs;
  for (size_t e = 0; e < events.size(); ++e)
  {
    const auto ms = e / 2 * pairMs + (e % 2 ? 10.0 : 0.0);
    follower.OnNoteOn(events[e].pitches.front(), ms);
    EXPECT_EQ(follower.FollowedEvent(), e) << "event " << e;
  }
}

TEST(PerformanceFollowerTests, CorrectedNotesAreFollowed)
{
  // Single notes, stepwise, every other one first struck a semitone off and
  // corrected 80 ms later.
  std::vector<ScoreEvent> events;
  for (auto i = 0; i < 16; ++i)
    events.push_back({i * quarter, {CMajor(i)}, i * quarter});
  PerformanceFollower follower{events, nominalTicksPerMs};
  const auto quarterMs = quarter / nominalTicksPerMs;
  for (size_t e = 0; e < events.size(); ++e)
  {
    const auto ms = e * quarterMs;
    const auto pitch = events[e].pitches.front();
    if (e % 2)
      follower.OnNoteOn(pitch + 1, ms);
    follower.OnNoteOn(pitch, ms + (e % 2 ? 80.0 : 0.0));
    EXPECT_EQ(follower.FollowedEvent(), e) << "event " << e;
  }
}

TEST(PerformanceFollowerTests, TrillOverChordsIsFollowed)
{
  // A whole-note trill on D in the right hand over a chord on each beat in
  // the left (G, C, D, G), then a C: the trill goes on through the chords.
  const std::vector<std::vector<int>> chords{
      {43, 47, 50}, {48, 52, 55}, {50, 54, 57}, {43, 47, 50}};
  std::vector<ScoreEvent> events;
  for (auto beat = 0; beat < 4; ++beat)
    events.push_back({beat * quarter, chords[beat], beat * quarter});
  events[0].pitches.insert(events[0].pitches.end(), {74, 76});
  events[0].trillPitches = {74, 76};
  events[0].trillEnd = bar;
  events.push_back({bar, {72}, bar});
  SpreadTrills(events);

  // The trill at 32nds, the chords' notes 10 ms apart.
  const auto quarterMs = quarter / nominalTicksPerMs;
  std::vector<std::pair<double, int>> take;
  for (auto i = 0; i < 32; ++i)
    take.emplace_back(i * quarterMs / 8, i % 2 ? 76 : 74);
  for (auto beat = 0; beat < 4; ++beat)
    for (size_t n = 0; n < chords[beat].size(); ++n)
      take.emplace_back(beat * quarterMs + 5.0 + 10.0 * n, chords[beat][n]);
  take.emplace_back(4 * quarterMs, 72);
  std::sort(take.begin(), take.end());

  PerformanceFollower follower{events, nominalTicksPerMs};
  for (const auto &[ms, pitch] : take)
  {
    follower.OnNoteOn(pitch, ms);
    const auto beat = std::min(static_cast<size_t>(ms / quarterMs), size_t{4});
    if (ms - beat * quarterMs >= 30.0 || beat == 4)
      // Past the chord's notes: where it is.
      EXPECT_EQ(follower.FollowedEvent(), beat) << pitch << " at " << ms;
  }
}

TEST(PerformanceFollowerTests, MistakesDontDerail)
{
  PerformanceStyle style;
  style.drift = 0.02;
  style.chordSpreadMs = 30.0;
  style.miss = 0.05;
  style.wrong = 0.05;
  style.extra = 0.03;
  const auto r = FollowTakes(style);
  EXPECT_GE(r.accuracy, 0.9);
  EXPECT_GE(r.followedAccuracy, 0.9);
}

TEST(PerformanceFollowerTests, RestartIsFound)
{
  const auto events = Piece();
  PerformanceStyle style;
  style.chordSpreadMs = 20.0;
  // Back from bar 20 (in B) to bar 18, then from bar 6 (in A, repeated in
  // bars 8-15 and 24-31) to bar 4.
  style.restarts = {{BarStart(events, 6), BarStart(events, 4)},
                    {BarStart(events, 20), BarStart(events, 18)}};
  const auto take = Perform(events, style, 1);
  std::vector<PerformanceFollower::Estimate> estimates;
  Follow(events, take, &estimates);

  // The restarts are where the performer paused.
  auto restarts = 0;
  for (size_t index = 1; index < take.size(); ++index)
  {
    if (take[index].ms - take[index - 1].ms < 0.9 * style.restartPauseMs)
      continue;
    ++restarts;
    // Found within a few notes, and kept.
    constexpr auto maxNotes = 6u;
    for (auto i = index + maxNotes; i < index + 40; ++i)
      EXPECT_EQ(estimates[i].event, static_cast<size_t>(take[i].truth))
          << "restart " << restarts << ", note " << i - index;
  }
  EXPECT_EQ(restarts, 2);
}

/**
 * Back from bar 23 to bar 16, both in B, further back than a jump goes
 * unless the performer sees it; the notes after the restart, as the follower
 * estimates them, when it sees bars [visibleBegin, visibleEnd).
 */
std::vector<std::pair<size_t, size_t>> FarRestart(int visibleBegin,
                                                  int visibleEnd)
{
  const auto events = Piece();
  PerformanceStyle style;
  style.restarts = {{BarStart(events, 23), BarStart(events, 16)}};
  const auto take = Perform(events, style, 1);
  PerformanceFollower follower{events, nominalTicksPerMs};
  follower.SetVisibleTicks(visibleBegin * bar, visibleEnd * bar);
  std::vector<std::pair<size_t, size_t>> after;
  for (size_t i = 0; i < take.size(); ++i)
  {
    const auto estimate = follower.OnNoteOn(take[i].pitch, take[i].ms);
    if (i > 0 && take[i].ms - take[i - 1].ms > 0.9 * style.restartPauseMs)
      after.emplace_back(estimate.event, take[i].truth);
    else if (!after.empty() && after.size() < 40)
      after.emplace_back(estimate.event, take[i].truth);
  }
  return after;
}

TEST(PerformanceFollowerTests, RestartFarBackIsFoundIfVisible)
{
  const auto after = FarRestart(16, 24);
  ASSERT_EQ(after.size(), 40u);
  for (auto i = 6u; i < after.size(); ++i)
    EXPECT_EQ(after[i].first, after[i].second) << "note " << i;
}

TEST(PerformanceFollowerTests, RestartIsNotFoundIfNotVisible)
{
  const auto after = FarRestart(20, 24);
  ASSERT_EQ(after.size(), 40u);
  auto right = 0;
  for (const auto &[estimated, truth] : after)
    right += estimated == truth;
  EXPECT_LT(right, 10);
}

TEST(PerformanceFollowerTests, RestartsInRepeatedPassagesAreFollowed)
{
  // Section A is played thrice alike: the follower must prefer going back a
  // little over going to another repeat of the passage.
  const auto events = Piece();
  PerformanceStyle style;
  style.chordSpreadMs = 30.0;
  style.miss = style.wrong = 0.05;
  style.restarts = {{BarStart(events, 6), BarStart(events, 4)},
                    {BarStart(events, 27), BarStart(events, 24)}};
  EXPECT_GE(FollowTakes(style).followedAccuracy, 0.95);
}

TEST(PerformanceFollowerTests, ResetStartsWhereTold)
{
  const auto events = Piece();
  PerformanceFollower follower{events, nominalTicksPerMs};
  // The piece's A section starts three times alike: the reset decides.
  const auto second = static_cast<size_t>(
      std::find_if(events.begin(), events.end(),
                   [](const ScoreEvent &e) { return e.tick == 24 * bar; }) -
      events.begin());
  follower.Reset(second);
  auto ms = 0.0;
  for (auto e = second; e < second + 20; ++e)
  {
    if (e > second)
      ms += (events[e].tick - events[e - 1].tick) / nominalTicksPerMs;
    for (const auto pitch : events[e].pitches)
      EXPECT_EQ(follower.OnNoteOn(pitch, ms).event, e);
  }
}

TEST(PerformanceFollowerTests, TrillIsPlayedAsLongAsTheNote)
{
  // C, a whole-note trill on D (one event of its two notes, see
  // GetScoreEvents), then E: however many trill notes, and however fast.
  const std::vector<ScoreEvent> events{
      {0, {72}},
      {quarter, {74, 76}, quarter, {74, 76}, 5 * quarter},
      {5 * quarter, {76}},
      {6 * quarter, {77}}};
  for (const auto noteMs : {50.0, 70.0, 110.0})
  {
    PerformanceFollower follower{events, nominalTicksPerMs};
    const auto msPerQuarter = quarter / nominalTicksPerMs;
    EXPECT_EQ(follower.OnNoteOn(72, 0.0).event, 0u);
    auto ms = msPerQuarter;
    for (auto i = 0; ms < 5 * msPerQuarter - noteMs; ++i, ms += noteMs)
    {
      const auto pitch = i % 2 ? 76 : 74;
      const auto event = follower.OnNoteOn(pitch, ms).event;
      // An E late in the trill may as well be the E after it, the trill cut
      // short; a D can't.
      if (pitch == 74 || ms < 4 * msPerQuarter)
      {
        EXPECT_EQ(event, 1u) << noteMs << " ms trill note " << i;
      }
    }
    EXPECT_EQ(follower.OnNoteOn(76, 5 * msPerQuarter).event, 2u) << noteMs;
    EXPECT_EQ(follower.OnNoteOn(77, 6 * msPerQuarter).event, 3u) << noteMs;
    EXPECT_NEAR(follower.TicksPerMs() / nominalTicksPerMs, 1.0, 0.1) << noteMs;
  }
}

/**
 * Not a test as such: prints how well the follower does across styles of
 * playing, for tuning.
 */
TEST(PerformanceFollowerTests, Report)
{
  struct Row
  {
    const char *name;
    PerformanceStyle style;
  };
  const auto sloppy = [](double rate)
  {
    PerformanceStyle s;
    s.drift = 0.03;
    s.chordSpreadMs = 30.0;
    s.miss = rate;
    s.wrong = rate;
    s.extra = rate / 2;
    return s;
  };
  const auto events = Piece();
  const auto restarting = [&](double rate)
  {
    auto s = sloppy(rate);
    s.restarts = {{BarStart(events, 6), BarStart(events, 4)},
                  {BarStart(events, 13), BarStart(events, 12)},
                  {BarStart(events, 21), BarStart(events, 18)},
                  {BarStart(events, 27), BarStart(events, 24)}};
    return s;
  };
  const std::vector<Row> rows = {
      {"clean, uneven chords", {1.0, 0.0, 0.0, 0.0, 0.0, 40.0, 1000.0, {}}},
      {"slow (x0.6)", {0.6, 0.0, 0.0, 0.0, 0.0, 20.0, 1000.0, {}}},
      {"fast (x1.6)", {1.6, 0.0, 0.0, 0.0, 0.0, 20.0, 1000.0, {}}},
      {"tempo drift 5%", {1.0, 0.05, 0.0, 0.0, 0.0, 20.0, 1000.0, {}}},
      {"errors 2%", sloppy(0.02)},
      {"errors 5%", sloppy(0.05)},
      {"errors 10%", sloppy(0.10)},
      {"errors 20%", sloppy(0.20)},
      {"restarts", restarting(0.0)},
      {"restarts, errors 5%", restarting(0.05)},
      {"restarts, errors 10%", restarting(0.10)},
  };
  std::printf("%-22s %9s %9s %7s\n", "style", "estimate", "followed", "tempo");
  for (const auto &row : rows)
  {
    const auto r = FollowTakes(row.style);
    std::printf("%-22s %8.1f%% %8.1f%% %7.2f\n", row.name, 100 * r.accuracy,
                100 * r.followedAccuracy, r.finalTempoRatio);
  }
}
} // namespace dgk
