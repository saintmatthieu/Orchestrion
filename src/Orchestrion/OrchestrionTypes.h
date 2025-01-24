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
  explicit PastChord(const IChord *past) : past{past} { assert(past); }
  const IChord *const past;
  const IChord *operator->() const { return past; }
};

struct PresentChord
{
  explicit PresentChord(const IChord *present) : present{present}
  {
    assert(present);
  }
  const IChord *const present;
  const IChord *operator->() const { return present; }
};

struct FutureChord
{
  explicit FutureChord(const IChord *future) : future{future}
  {
    assert(future);
  }
  const IChord *const future;
  const IChord *operator->() const { return future; }
};

struct PastChordAndPresentChord
{
  PastChordAndPresentChord(const IChord *past, const IChord *present)
      : past{past}, present{present}
  {
    assert(past);
    assert(present);
  }
  const IChord *const past;
  const IChord *const present;
};

struct PastChordAndPresentRest
{
  PastChordAndPresentRest(const IChord *past, const IRest *present)
      : past{past}, present{present}
  {
    assert(past);
    assert(present);
  }
  const IChord *const past;
  const IRest *const present;
};

struct PastChordAndFutureChord
{
  PastChordAndFutureChord(const IChord *past, const IChord *future)
      : past{past}, future{future}
  {
    assert(past);
    assert(future);
  }
  const IChord *const past;
  const IChord *const future;
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