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

using ChordPtr = std::shared_ptr<IChord>;
using Staff = std::map<int /*voice*/, std::vector<ChordPtr>>;

struct ChordActivationChange
{
  ChordActivationChange(std::vector<const IChord *> deactivated,
                        const IChord *activated)
      : deactivated{std::move(deactivated)}, activated{activated}
  {
  }

  ChordActivationChange() : deactivated{}, activated{nullptr} {}

  const std::vector<const IChord *> deactivated;
  const IChord *const activated;
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