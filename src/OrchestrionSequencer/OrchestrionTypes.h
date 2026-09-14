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

/**
 * A stretch of depressed pedal, [onTick, offTick), in ticks with repeats.
 */
struct PedalSpan
{
  int onTick = 0;
  int offTick = 0;
};

/**
 * The pedal spans of a part, sorted and non-overlapping.
 */
using PedalSequence = std::vector<PedalSpan>;

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

  int value;
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

  NoteEventType type;
  TrackIndex track;
  int pitch;
  float velocity;
};

using NoteEvents = std::vector<NoteEvent>;

struct InstrumentIndex
{
  explicit InstrumentIndex(int value) : value{value} {}
  int value;
};

struct PedalEvent
{
  PedalEvent(InstrumentIndex instrument, bool on)
      : instrument{std::move(instrument)}, on{on}
  {
  }

  InstrumentIndex instrument;
  bool on;
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

/**
 * A grace note: its pitches, and the value it is to be played at, in ticks.
 */
struct GraceNote
{
  std::vector<int> pitches;
  int ticks = 0;
};

/**
 * What an ornament sign asks for.
 */
enum class OrnamentSign
{
  none,
  /**
   * A figure of definite shape (a turn, a mordent, the short trill's
   * flutter, ...): at its natural speed when the note has room for it, the
   * main note then held; compressed to fill the note otherwise, running into
   * the next.
   */
  figure,
  /**
   * The trill proper: an alternation kept up for as long as the note lasts.
   */
  trill,
};

/**
 * How an ornamented chord (a trill, a mordent, a turn, grace notes, ...)
 * unfolds from the one gesture that strikes it — the notes only. How long
 * each lasts is the sequencer's business, decided when the chord is struck
 * from the performer's tempo. Nothing is anticipated: the first note sounds
 * at the strike, so an ornament meant to fall before the beat is for the
 * player to anticipate, by striking a little early — as the automatic player
 * does for the grace notes (see IOrchestrionSequencer::WhatToPlayNext).
 */
struct Ornament
{
  /**
   * Grace notes before the chord, in order, each at the value it plays at:
   * 32nds when they resolve a trill (the chord struck before is trilled), a
   * lone unslashed one leaning on the chord by step (an appoggiatura) its
   * written value, any other — a crushed one, a run, one leaping to the
   * chord or repeating the note before — a 64th. Together they take half the
   * chord at most.
   */
  std::vector<GraceNote> gracesBefore;
  /**
   * Whether the graces before are an appoggiatura: it falls on the beat and
   * takes its time from the chord, so a player does not anticipate it, unlike
   * the quick graces, runs and trill resolutions that come before the beat.
   */
  bool appoggiatura = false;
  OrnamentSign sign = OrnamentSign::none;
  /**
   * The notes the sign spells out, the main note included where it falls
   * (a turn: main, upper, main, lower, main). For a trill, the two notes it
   * alternates, the first struck first.
   */
  std::vector<std::vector<int>> figure;
  /**
   * Grace notes after the chord — 32nds resolving a trill, 64ths otherwise:
   * played when the gesture that leaves the chord comes, before whatever it
   * moves on to.
   */
  std::vector<GraceNote> gracesAfter;
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