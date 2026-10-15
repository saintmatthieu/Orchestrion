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

#include "IChord.h"
#include "IRest.h"
#include "internal/OrnamentGesture.h"
#include "internal/OrnamentSchedule.h"
#include "internal/ScoreEvents.h"

#include <memory>
#include <optional>
#include <vector>

namespace dgk
{
namespace
{
constexpr int quarter = 480;
constexpr int half = 2 * quarter;
constexpr int bar = 4 * quarter;

class FakeChord : public IChord
{
public:
  FakeChord(int begin, int end, std::vector<int> pitches,
            std::optional<Ornament> ornament = std::nullopt)
      : m_begin{begin}, m_end{end}, m_pitches{std::move(pitches)},
        m_ornament{std::move(ornament)}
  {
  }

  const IChord *AsChord() const override { return this; }
  IChord *AsChord() override { return this; }
  const IRest *AsRest() const override { return nullptr; }
  IRest *AsRest() override { return nullptr; }
  Tick GetBeginTick() const override { return {m_begin, m_begin}; }
  Tick GetEndTick() const override { return {m_end, m_end}; }
  std::vector<int> GetPitches() const override { return m_pitches; }
  float GetVelocity() const override { return 0.f; }
  void SetVelocity(float) override {}
  std::optional<float> GetDynamicVelocity() const override
  {
    return std::nullopt;
  }
  const Ornament *GetOrnament() const override
  {
    return m_ornament ? &*m_ornament : nullptr;
  }
  double GetNominalBpm() const override { return 60.0; }
  const mu::engraving::Chord *GetEngravingChord() const override
  {
    return nullptr;
  }

private:
  const int m_begin;
  const int m_end;
  const std::vector<int> m_pitches;
  const std::optional<Ornament> m_ornament;
};

class FakeRest : public IRest
{
public:
  FakeRest(int begin, int end) : m_begin{begin}, m_end{end} {}
  const IChord *AsChord() const override { return nullptr; }
  IChord *AsChord() override { return nullptr; }
  const IRest *AsRest() const override { return this; }
  IRest *AsRest() override { return this; }
  Tick GetBeginTick() const override { return {m_begin, m_begin}; }
  Tick GetEndTick() const override { return {m_end, m_end}; }

private:
  const int m_begin;
  const int m_end;
};

Ornament Trill()
{
  Ornament ornament;
  ornament.sign = OrnamentSign::trill;
  ornament.figure = {{72}, {74}};
  return ornament;
}

Ornament Turn()
{
  Ornament ornament;
  ornament.sign = OrnamentSign::figure;
  ornament.figure = {{72}, {74}, {72}, {71}, {72}};
  return ornament;
}

std::vector<int> Ticks(const std::vector<ScoreEvent> &events)
{
  std::vector<int> ticks;
  for (const auto &e : events)
    ticks.push_back(e.tick);
  return ticks;
}
} // namespace

TEST(ScoreEventsTests, VoicesAndStavesAreMergedIntoOnsets)
{
  const Staff rightHand{
      {0,
       {std::make_shared<FakeChord>(0, quarter, std::vector<int>{76}),
        std::make_shared<FakeRest>(quarter, half),
        std::make_shared<FakeChord>(half, bar, std::vector<int>{79})}},
      {1, {std::make_shared<FakeChord>(0, bar, std::vector<int>{72})}}};
  const Staff leftHand{
      {0,
       {std::make_shared<FakeChord>(0, half, std::vector<int>{48, 55}),
        std::make_shared<FakeChord>(half, bar + half, std::vector<int>{50}),
        std::make_shared<FakeChord>(bar + half, 2 * bar,
                                    std::vector<int>{48})}}};

  const auto events = GetScoreEvents({&rightHand, &leftHand});

  ASSERT_EQ(events.size(), 3u);
  EXPECT_EQ(events[0].tick, 0);
  EXPECT_EQ(events[0].pitches, (std::vector<int>{48, 55, 72, 76}));
  EXPECT_EQ(events[1].tick, half);
  EXPECT_EQ(events[1].pitches, (std::vector<int>{50, 79}));
  EXPECT_EQ(events[2].tick, bar + half);
  EXPECT_EQ(events[2].pitches, (std::vector<int>{48}));
}

TEST(ScoreEventsTests, FigureIsWrittenOut)
{
  const Staff staff{
      {0,
       {std::make_shared<FakeChord>(0, half, std::vector<int>{72}, Turn()),
        std::make_shared<FakeChord>(half, bar, std::vector<int>{76})}}};

  const auto events = GetScoreEvents({&staff});

  ASSERT_EQ(events.size(), 6u);
  const std::vector<std::vector<int>> pitches{{72}, {74}, {72},
                                              {71}, {72}, {76}};
  for (size_t i = 0; i < events.size(); ++i)
    EXPECT_EQ(events[i].pitches, pitches[i]) << i;
  EXPECT_EQ(events[0].tick, 0);
  EXPECT_EQ(events[5].tick, half);
}

TEST(ScoreEventsTests, TrillIsOneEvent)
{
  const Staff staff{
      {0,
       {std::make_shared<FakeChord>(0, half, std::vector<int>{72}, Trill()),
        std::make_shared<FakeChord>(half, bar, std::vector<int>{76})}}};

  const auto events = GetScoreEvents({&staff});

  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].tick, 0);
  EXPECT_EQ(events[0].pitches, (std::vector<int>{72, 74}));
  EXPECT_EQ(events[0].trillPitches, (std::vector<int>{72, 74}));
  EXPECT_EQ(events[0].trillEnd, half);
  EXPECT_EQ(events[1].tick, half);
  EXPECT_TRUE(events[1].trillPitches.empty());
}

TEST(ScoreEventsTests, TrillIsCarriedOverTheOtherVoicesOnsets)
{
  // A whole-note trill on C, while the other voice moves in quarters.
  const Staff staff{
      {0, {std::make_shared<FakeChord>(0, bar, std::vector<int>{72}, Trill())}},
      {1,
       {std::make_shared<FakeChord>(0, quarter, std::vector<int>{67}),
        std::make_shared<FakeChord>(quarter, half, std::vector<int>{65}),
        std::make_shared<FakeChord>(half, half + quarter, std::vector<int>{64}),
        std::make_shared<FakeChord>(half + quarter, bar,
                                    std::vector<int>{65})}}};

  const auto events = GetScoreEvents({&staff});

  ASSERT_EQ(events.size(), 4u);
  for (const auto &event : events)
  {
    EXPECT_EQ(event.trillPitches, (std::vector<int>{72, 74})) << event.tick;
    EXPECT_EQ(event.trillEnd, bar) << event.tick;
  }
  EXPECT_EQ(events[1].pitches, (std::vector<int>{65}));
}

TEST(ScoreEventsTests, OrnamentsWrittenOutForTheManualModeGiveTheSameEvents)
{
  const auto turn =
      std::make_shared<FakeChord>(0, half, std::vector<int>{72}, Turn());
  const auto trill =
      std::make_shared<FakeChord>(half, bar, std::vector<int>{76}, Trill());
  const Staff chords{{0, {turn, trill}}};

  // What the sequencer factory makes of them in the manual mode.
  std::vector<ChordRestPtr> gestures;
  for (const std::shared_ptr<IChord> &chord :
       std::vector<std::shared_ptr<IChord>>{turn, trill})
  {
    Tick from = chord->GetBeginTick();
    for (const WrittenNote &note : WriteOutOrnament(*chord))
    {
      Tick to = from;
      to += note.ticks;
      gestures.push_back(
          std::make_shared<OrnamentGesture>(chord, note.pitches, from, to));
      from = to;
    }
  }
  const Staff writtenOut{{0, gestures}};

  const auto expected = GetScoreEvents({&chords});
  const auto actual = GetScoreEvents({&writtenOut});
  EXPECT_EQ(Ticks(actual), Ticks(expected));
  ASSERT_EQ(actual.size(), expected.size());
  for (size_t i = 0; i < actual.size(); ++i)
    EXPECT_EQ(actual[i].pitches, expected[i].pitches) << i;
}
} // namespace dgk
