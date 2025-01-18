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
class IChordRest;

using ChordRestPtr = std::shared_ptr<IChordRest>;
using Staff = std::map<int /*voice*/, std::vector<ChordRestPtr>>;

// clang-format off
/*!
 * \brief There are four gesture transitions possible: off->on, on->off, on->on (legato) and off->off (e.g. succession of key releases)
 * The possible chord transitions for each are as follows:
 * -- OFF -> ON :
 *     - implicit rest (nullptr) to chord like at the beginning of a piece or when playing non-legato
 *     - rest to chord
 * -- ON -> OFF :
 *     - chord to implicit rest (nullptr) like at the end of a piece or when playing non-legato
 * -- ON -> ON (legato) :
 *    - chord to chord
 *    - chord to chord over skipped rest
 * -- OFF -> OFF :
 *    - rest to rest
 * The ChordTransition represents all these possibilities.
 */
// clang-format on
struct ChordTransition
{
  struct DeactivatedChord
  {
    explicit DeactivatedChord(const IChord *value) : value{value} {}
    const IChord *const value;
    operator bool() const { return value != nullptr; }
    const IChord *operator->() const { return value; }
  };
  struct SkippedRest
  {
    explicit SkippedRest(const IRest *value) : value{value} {}
    const IRest *const value;
    operator bool() const { return value != nullptr; }
    const IRest *operator->() const { return value; }
  };
  struct ActivatedChordRest
  {
    explicit ActivatedChordRest(const IChordRest *value) : value{value} {}
    const IChordRest *const value;
    operator bool() const { return value != nullptr; }
    const IChordRest *operator->() const { return value; }
    const IChord *AsChord() const { return value ? value->AsChord() : nullptr; }
  };
  struct NextChord
  {
    explicit NextChord(const IChord *value) : value{value} {}
    const IChord *const value;
    operator bool() const { return value != nullptr; }
    const IChord *operator->() const { return value; }
  };

  ChordTransition()
      : deactivatedChord{nullptr}, skippedRest{nullptr},
        activatedChordRest{nullptr}, nextChord{nullptr}
  {
  }

  ChordTransition(NextChord nextChord)
      : deactivatedChord{nullptr}, skippedRest{nullptr},
        activatedChordRest{nullptr}, nextChord{std::move(nextChord)}
  {
  }

  ChordTransition(DeactivatedChord deactivatedChord, NextChord nextChord)
      : deactivatedChord{std::move(deactivatedChord)}, skippedRest{nullptr},
        activatedChordRest{nullptr}, nextChord{std::move(nextChord)}
  {
  }

  ChordTransition(DeactivatedChord deactivatedChord, SkippedRest skippedRest,
                  ActivatedChordRest activatedChordRest)
      : deactivatedChord{std::move(deactivatedChord)},
        skippedRest{std::move(skippedRest)},
        activatedChordRest{std::move(activatedChordRest)}, nextChord{nullptr}
  {
  }

  ChordTransition(DeactivatedChord deactivatedChord,
                  ActivatedChordRest activatedChordRest)
      : deactivatedChord{std::move(deactivatedChord)}, skippedRest{nullptr},
        activatedChordRest{std::move(activatedChordRest)}, nextChord{nullptr}
  {
  }

  ChordTransition(ActivatedChordRest activatedChordRest)
      : deactivatedChord{nullptr}, skippedRest{nullptr},
        activatedChordRest{std::move(activatedChordRest)}, nextChord{nullptr}
  {
  }

  ChordTransition(SkippedRest skippedRest,
                  ActivatedChordRest activatedChordRest)
      : deactivatedChord{nullptr}, skippedRest{std::move(skippedRest)},
        activatedChordRest{std::move(activatedChordRest)}, nextChord{nullptr}
  {
  }

  ChordTransition(DeactivatedChord deactivatedChord, SkippedRest skippedRest,
                  ActivatedChordRest activatedChordRest, NextChord nextChord)
      : deactivatedChord{std::move(deactivatedChord)},
        skippedRest{std::move(skippedRest)},
        activatedChordRest{std::move(activatedChordRest)},
        nextChord{std::move(nextChord)}
  {
  }

  const DeactivatedChord deactivatedChord;
  const SkippedRest skippedRest;
  const ActivatedChordRest activatedChordRest;
  const NextChord nextChord;
};

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

  int withRepeats;
  int withoutRepeats;
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