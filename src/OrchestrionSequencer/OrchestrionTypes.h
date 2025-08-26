/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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
#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <variant>
#include <vector>

namespace dgk
{

static constexpr auto numVoices = 4;

struct PedalSequenceItem
{
  int tick = 0;
  bool down = false;
};

using PedalSequence = std::vector<PedalSequenceItem>;

class IChord;
class IRest;
class IMelodySegment;

using ChordRestPtr = std::shared_ptr<IMelodySegment>;
using Staff = std::map<int /*voice*/, std::vector<ChordRestPtr>>;

struct PastChord
{
  explicit PastChord(const IChord *chord) : chord{chord} { assert(chord); }
  const IChord *const chord;
  const IChord *operator->() const { return chord; }
};

struct PresentChord
{
  explicit PresentChord(const IChord *chord) : chord{chord} { assert(chord); }
  const IChord *const chord;
  const IChord *operator->() const { return chord; }
};

struct FutureChord
{
  explicit FutureChord(const IChord *chord) : chord{chord} { assert(chord); }
  const IChord *const chord;
  const IChord *operator->() const { return chord; }
};

struct PastChordAndPresentChord
{
  PastChordAndPresentChord(const IChord *pastChord, const IChord *presentChord)
      : pastChord{pastChord}, presentChord{presentChord}
  {
    assert(pastChord);
    assert(presentChord);
  }
  const IChord *const pastChord;
  const IChord *const presentChord;
};

struct PastChordAndPresentRest
{
  PastChordAndPresentRest(const IChord *pastChord, const IRest *presentRest)
      : pastChord{pastChord}, presentRest{presentRest}
  {
    assert(pastChord);
    assert(presentRest);
  }
  const IChord *const pastChord;
  const IRest *const presentRest;
};

struct PastChordAndFutureChord
{
  PastChordAndFutureChord(const IChord *pastChord, const IChord *futureChord)
      : pastChord{pastChord}, futureChord{futureChord}
  {
    assert(pastChord);
    assert(futureChord);
  }
  const IChord *const pastChord;
  const IChord *const futureChord;
};

using ChordTransition = std::variant<PastChord,                //
                                     PresentChord,             //
                                     FutureChord,              //
                                     PastChordAndPresentChord, //
                                     PastChordAndFutureChord,  //
                                     PastChordAndPresentRest   //
                                     >;

// helper type for the visitor #4
template <class... Ts> struct overloaded : Ts...
{
  using Ts::operator()...;
};

// explicit deduction guide (not needed as of C++20)
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

const IChord *GetPastChord(const ChordTransition &);
const IChord *GetPresentChord(const ChordTransition &);
IChord *GetPresentChord(ChordTransition &);
const IMelodySegment *GetPresentThing(const ChordTransition &);
const IChord *GetFutureChord(const ChordTransition &);

template <typename T> const T *Get(const ChordTransition &transition)
{
  return std::get_if<T>(&transition);
}

struct TrackIndex
{
  explicit TrackIndex(int value) : value{value} {}

  TrackIndex(int staff, int voice) : value{staff * numVoices + voice}
  {
    assert(value >= 0);
  }

  TrackIndex() : value{-1} {}

  const int value;
  int voiceIndex() const { return value % numVoices; }
  int staffIndex() const { return value / numVoices; }

  bool operator==(const TrackIndex &rhs) const { return value == rhs.value; }
  bool operator!=(const TrackIndex &rhs) const { return !(*this == rhs); }
  bool operator<(const TrackIndex &rhs) const { return value < rhs.value; }
};

enum class NoteEventType
{
  noteOn,
  noteOff
};

struct NoteEvent
{
  NoteEvent(NoteEventType type, TrackIndex track, int pitch, float velocity)
      : type{type}, track{std::move(track)}, pitch{pitch}, velocity{velocity}
  {
  }

  // NoteEvent(const NoteEvent& other) = default;
  // NoteEvent& operator=(const NoteEvent& other) = default;
  // NoteEvent(NoteEvent&& other) = default;

  const NoteEventType type;
  const TrackIndex track;
  const int pitch;
  const float velocity;
};

using NoteEvents = std::vector<NoteEvent>;

struct InstrumentIndex
{
  explicit InstrumentIndex(int value) : value{value} {}
  const int value;
};

struct PedalEvent
{
  PedalEvent(InstrumentIndex instrument, bool on)
      : instrument{std::move(instrument)}, on{on}
  {
  }

  const InstrumentIndex instrument;
  const bool on;
};

using EventVariant = std::variant<NoteEvents, PedalEvent>;

struct Tick
{
  using value_type = int;

  Tick(int withRepeats, int withoutRepeats)
      : withRepeats{withRepeats}, withoutRepeats{withoutRepeats}
  {
  }

  Tick &operator+=(int tick)
  {
    withRepeats += tick;
    withoutRepeats += tick;
    return *this;
  }

  constexpr bool operator<(const Tick &rhs) const
  {
    // For a given voice, chords are uniquely positioned when accounting for
    // repeats. In other words, two or more chords may havye the same
    // tick-without-repeats value.
    return withRepeats < rhs.withRepeats;
  }

  constexpr bool operator>=(const Tick &rhs) const { return !(*this < rhs); }
  constexpr bool operator>(const Tick &rhs) const { return rhs < *this; }
  constexpr bool operator<=(const Tick &rhs) const { return !(*this > rhs); }

  value_type withRepeats;
  value_type withoutRepeats;
};

class Finally
{
public:
  Finally(std::function<void()> f) : m_f{std::move(f)} {}
  ~Finally() { m_f(); }

private:
  std::function<void()> m_f;
};
} // namespace dgk