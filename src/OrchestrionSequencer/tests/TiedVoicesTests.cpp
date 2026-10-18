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
#include "internal/TiedVoices.h"

#include <map>
#include <memory>
#include <vector>

namespace dgk
{
namespace
{
constexpr int quarter = 480;
constexpr int bar = 2 * quarter; // 2/4

constexpr int C5 = 72;
constexpr int E5 = 76;
constexpr int G5 = 79;
constexpr int A5 = 81;

class FakeChord : public IChord
{
public:
  FakeChord(int begin, int end, std::vector<int> pitches)
      : m_begin{begin}, m_end{end}, m_pitches{std::move(pitches)}
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
  const Ornament *GetOrnament() const override { return nullptr; }
  double GetNominalBpm() const override { return 60.0; }
  const mu::engraving::Chord *GetEngravingChord() const override
  {
    return nullptr;
  }

private:
  const int m_begin;
  const int m_end;
  const std::vector<int> m_pitches;
};

/** A voice of chords, and where their notes' ties end where they're tied. */
struct Voice
{
  std::vector<ChordRestPtr> chords;
  std::map<const IChord *, std::vector<TiedNote>> ties;

  const IChord *Add(int begin, int end, std::vector<int> pitches,
                    std::map<int, int> tiedEnds = {})
  {
    auto chord = std::make_shared<FakeChord>(begin, end, pitches);
    chords.push_back(chord);
    if (!tiedEnds.empty())
      for (const int pitch : pitches)
      {
        const int to = tiedEnds.count(pitch) ? tiedEnds.at(pitch) : end;
        ties[chord.get()].push_back({pitch, nullptr, {to, to}});
      }
    return chord.get();
  }
};

void Split(Staff &staff, const Voice &voice)
{
  SplitTiedVoices(
      staff,
      [&](const IChord &chord)
      {
        const auto it = voice.ties.find(&chord);
        return it == voice.ties.end() ? std::vector<TiedNote>{} : it->second;
      },
      [](const ChordRestPtr &, const IMelodySegment &) {},
      [](const IMelodySegment &) {});
}

struct Seen
{
  int begin = 0;
  int end = 0;
  std::vector<int> pitches; // empty for a rest
  bool operator==(const Seen &) const = default;
};

std::vector<Seen> See(const std::vector<ChordRestPtr> &sequence)
{
  std::vector<Seen> seen;
  for (const auto &item : sequence)
    seen.push_back(
        {item->GetBeginTick().withRepeats, item->GetEndTick().withRepeats,
         item->AsChord() ? item->AsChord()->GetPitches() : std::vector<int>{}});
  return seen;
}
} // namespace

// Mozart, K. 265, Var. II, bars 2-3: the upper voice's C5+G5 on bar 2's second
// beat is tied over into the inner voices, the G5 until bar 3's second beat,
// the C5 until bar 5's, while the upper voice strikes A5 on bar 3.
TEST(TiedVoices, NotesTiedIntoOtherVoicesGetLanesOfTheirOwn)
{
  const int bar2 = bar, bar3 = 2 * bar, bar5 = 4 * bar;
  Voice voice;
  voice.Add(bar2, bar2 + quarter, {C5, G5});
  voice.Add(bar2 + quarter, bar3, {C5, G5},
            {{G5, bar3 + quarter}, {C5, bar5 + quarter}});
  voice.Add(bar3, bar3 + bar, {A5});
  Staff staff{{0, voice.chords}};

  Split(staff, voice);

  ASSERT_EQ(staff.size(), 3u);
  // Nothing of the tied chord stays: the voice rests until its A5.
  EXPECT_EQ(See(staff.at(0)),
            (std::vector<Seen>{{bar2, bar2 + quarter, {C5, G5}},
                               {bar2 + quarter, bar3, {}},
                               {bar3, bar3 + bar, {A5}}}));
  EXPECT_EQ(See(staff.at(numVoices)),
            (std::vector<Seen>{{bar2 + quarter, bar3 + quarter, {G5}}}));
  EXPECT_EQ(See(staff.at(2 * numVoices)),
            (std::vector<Seen>{{bar2 + quarter, bar5 + quarter, {C5}}}));
}

// Var. II, bars 4-5: the E5 is tied into a chord of its voice with more notes,
// which a voice's chord can't be tied to: it lasts to the end of its tie.
TEST(TiedVoices, ANoteTiedOnInItsVoiceKeepsItsWholeLength)
{
  const int bar4 = 3 * bar, bar5 = 4 * bar;
  Voice voice;
  voice.Add(bar4 + quarter, bar5, {E5}, {{E5, bar5 + quarter}});
  voice.Add(bar5 + quarter, bar5 + bar, {G5});
  Staff staff{{1, voice.chords}};

  Split(staff, voice);

  ASSERT_EQ(staff.size(), 1u);
  EXPECT_EQ(See(staff.at(1)),
            (std::vector<Seen>{{bar4 + quarter, bar5 + quarter, {E5}},
                               {bar5 + quarter, bar5 + bar, {G5}}}));
}

TEST(TiedVoices, ChordsEndingTogetherAreLeftAsTheyAre)
{
  Voice voice;
  const IChord *tied = voice.Add(0, quarter, {C5, G5}, {{C5, quarter}});
  voice.Add(quarter, bar, {A5});
  Staff staff{{0, voice.chords}};

  Split(staff, voice);

  ASSERT_EQ(staff.size(), 1u);
  ASSERT_EQ(staff.at(0).size(), 2u);
  EXPECT_EQ(staff.at(0).front()->AsChord(), tied);
}

TEST(TiedVoices, AFreedLaneIsUsedAgain)
{
  Voice voice;
  voice.Add(0, quarter, {C5, G5}, {{C5, bar + quarter}});
  voice.Add(quarter, bar, {A5});
  voice.Add(2 * bar, 2 * bar + quarter, {C5, G5}, {{C5, 3 * bar + quarter}});
  voice.Add(2 * bar + quarter, 3 * bar, {A5});
  Staff staff{{0, voice.chords}};

  Split(staff, voice);

  ASSERT_EQ(staff.size(), 2u);
  // The G5s stay; the C5s share a lane, resting in between.
  EXPECT_EQ(See(staff.at(0)),
            (std::vector<Seen>{{0, quarter, {G5}},
                               {quarter, bar, {A5}},
                               {2 * bar, 2 * bar + quarter, {G5}},
                               {2 * bar + quarter, 3 * bar, {A5}}}));
  EXPECT_EQ(See(staff.at(numVoices)),
            (std::vector<Seen>{{0, bar + quarter, {C5}},
                               {bar + quarter, 2 * bar, {}},
                               {2 * bar, 3 * bar + quarter, {C5}}}));
}
} // namespace dgk
