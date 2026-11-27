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
#include "PerformanceAligner.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>

namespace dgk
{
namespace
{
// A Viterbi search over the score's events: the path of least cost through
// them that explains every performed note. Costs are negative
// log-likelihoods, roughly; only their differences matter.

/** Each event skipped (none of its notes played). */
constexpr double kSkipEvent = 6.0;
/** Each pitch of an event moved on from, or skipped, that wasn't played. */
constexpr double kMissingPitch = 3.0;
/** A note the score has no place for. */
constexpr double kExtra = 7.0;
/** Another note of a trill, while it goes on. */
constexpr double kTrillNote = 1.0;
/** Before the first note: each event the performance doesn't start with. */
constexpr double kLateStart = 2.0;
/** Farthest move on, in events; beyond that, only a jump over a repeat. */
constexpr int kMaxAdvance = 6;
/** Skipping a repeat: going on where the score goes on once played through. */
constexpr double kRepeatSkip = 15.0;
/**
 * A note joins its chord free within this long of the chord's first note,
 * then at kLateJoinPerMs. A rolled chord is spread at the performer's will.
 */
constexpr double kChordSpreadMs = 120.0;
constexpr double kRolledSpreadMs = 400.0;
constexpr double kLateJoinPerMs = 0.02;
/**
 * The timing of a move, from the event's first note to the next's: this
 * times the square of the log of the time it took over the time the tempo
 * gives it; kTimingFloorMs added to both, that near-simultaneous events
 * don't give extreme ratios.
 */
constexpr double kTimingWeight = 4.0;
constexpr double kTimingFloorMs = 30.0;
/**
 * The tempo is measured over moves of at least a sixteenth, that ornaments
 * (played at their own speed) don't skew it, and smoothed.
 */
constexpr int kMinTempoTicks = 120;
constexpr double kTempoSmoothing = 0.3;
constexpr double kMinTempoRatio = 0.25;
constexpr double kMaxTempoRatio = 4.0;
/** Paths this much costlier than the best are dropped. */
constexpr double kBeam = 60.0;
/**
 * The paths kept at each event, the cheapest: an event reached at another
 * time (another of the performer's notes its first) may turn out the better
 * by the timing of the notes after it.
 */
constexpr size_t kPathsPerEvent = 4;

enum class Step
{
  /** The note plays a pitch of the event. */
  match,
  /** It is another note of a trill going on at the event. */
  trill,
  /** The score has no place for it. */
  extra,
};

/** How a path explained a note: a link of its chain back to the start. */
struct Link
{
  int parent = -1;
  size_t note = 0;
  int event = 0;
  Step step = Step::match;
  /** Into the event's pitches, for a match. */
  int pitchIndex = 0;
};

/** The cheapest path found to an event (-1 before the first). */
struct Path
{
  double cost = 0.0;
  int event = -1;
  /** The onset of the event's first note. */
  double startMs = 0.0;
  double msPerTick = 0.0;
  /** The event's pitches played, by index. */
  uint64_t played = 0;
  /** Its last link, once chosen. */
  int link = -1;
  /** How it explains the note at hand, until chosen. */
  Link pending;
};

int IndexOf(const std::vector<int> &pitches, int pitch)
{
  const auto it = std::find(pitches.begin(), pitches.end(), pitch);
  return it == pitches.end() ? -1 : static_cast<int>(it - pitches.begin());
}

bool Contains(const std::vector<int> &pitches, int pitch)
{
  return IndexOf(pitches, pitch) >= 0;
}

int CountUnplayed(const ScoreEvent &event, uint64_t played)
{
  int count = 0;
  for (size_t i = 0; i < event.pitches.size(); ++i)
    if (!(played >> i & 1))
      ++count;
  return count;
}

/** An unplayed pitch of `event`, if `pitch` is one of them. */
int UnplayedIndex(const ScoreEvent &event, uint64_t played, int pitch)
{
  for (size_t i = 0; i < event.pitches.size(); ++i)
    if (event.pitches[i] == pitch && !(played >> i & 1))
      return static_cast<int>(i);
  return -1;
}

/** The event the trill of `pitch` going on at `event` begins at. */
size_t TrillStart(const std::vector<ScoreEvent> &events, size_t event,
                  int pitch)
{
  auto start = event;
  while (start > 0 && events[start - 1].trillEnd == events[event].trillEnd &&
         Contains(events[start - 1].trillPitches, pitch))
    --start;
  return start;
}
} // namespace

PerformanceAlignment AlignPerformance(const std::vector<ScoreEvent> &events,
                                      const std::vector<PerformedNote> &notes,
                                      double nominalTicksPerMs)
{
  PerformanceAlignment alignment;
  alignment.events.resize(events.size());
  for (size_t e = 0; e < events.size(); ++e)
    alignment.events[e].notes.assign(events[e].pitches.size(),
                                     PerformanceAlignment::none);
  if (events.empty() || notes.empty())
  {
    for (size_t j = 0; j < notes.size(); ++j)
      alignment.added.push_back(j);
    return alignment;
  }

  const auto numEvents = static_cast<int>(events.size());
  // What skipping events costs, and what leaving the rest of the score
  // unplayed does, as prefix sums.
  std::vector<double> skipCost(events.size() + 1, 0.0);
  std::vector<double> missingCost(events.size() + 1, 0.0);
  for (size_t e = 0; e < events.size(); ++e)
  {
    const double missing = kMissingPitch * events[e].pitches.size();
    skipCost[e + 1] = skipCost[e] + kSkipEvent + missing;
    missingCost[e + 1] = missingCost[e] + missing;
  }
  // Where the score goes on after an event, played through without repeats:
  // the events written next, in every pass.
  std::map<int, std::vector<int>> byWrittenTick;
  for (int e = 0; e < numEvents; ++e)
    byWrittenTick[events[e].writtenTick].push_back(e);

  const double nominalMsPerTick = 1.0 / nominalTicksPerMs;
  std::vector<Link> links;
  links.reserve(notes.size() * 8);
  std::vector<Path> paths;
  paths.push_back({0.0, -1, notes.front().onsetMs, nominalMsPerTick});

  for (size_t j = 0; j < notes.size(); ++j)
  {
    const PerformedNote &note = notes[j];
    std::map<int, std::vector<Path>> next;
    const auto offer = [&](const Path &path)
    {
      std::vector<Path> &kept = next[path.event];
      // In the same state (begun with the same note, the same of its pitches
      // played), the cheaper.
      for (Path &other : kept)
        if (other.startMs == path.startMs && other.played == path.played)
        {
          if (path.cost < other.cost)
            other = path;
          return;
        }
      if (kept.size() < kPathsPerEvent)
      {
        kept.push_back(path);
        return;
      }
      const auto costliest = std::max_element(kept.begin(), kept.end(),
                                              [](const Path &a, const Path &b)
                                              { return a.cost < b.cost; });
      if (path.cost < costliest->cost)
        *costliest = path;
    };

    for (const Path &path : paths)
    {
      const int from = path.event;
      const ScoreEvent *current = from >= 0 ? &events[from] : nullptr;
      if (current)
      {
        // A pitch of the event not played yet.
        if (const int index = UnplayedIndex(*current, path.played, note.pitch);
            index >= 0)
        {
          const double spread =
              current->rolled ? kRolledSpreadMs : kChordSpreadMs;
          const double late =
              std::max(0.0, note.onsetMs - path.startMs - spread);
          Path joined = path;
          joined.cost += kLateJoinPerMs * late;
          joined.played |= uint64_t{1} << index;
          joined.pending = {path.link, j, from, Step::match, index};
          offer(joined);
        }
        // Another note of a trill.
        if (Contains(current->trillPitches, note.pitch))
        {
          Path trilled = path;
          trilled.cost += kTrillNote;
          trilled.pending = {path.link, j, from, Step::trill};
          offer(trilled);
        }
        Path extra = path;
        extra.cost += kExtra;
        extra.pending = {path.link, j, from, Step::extra};
        offer(extra);
      }

      const double leftUnplayed =
          current ? kMissingPitch * CountUnplayed(*current, path.played) : 0.0;
      const auto moveTo = [&](int to, int ticks, double cost)
      {
        const int index = IndexOf(events[to].pitches, note.pitch);
        if (index < 0)
          return;
        Path moved = path;
        moved.cost += leftUnplayed + cost;
        if (current && ticks > 0)
        {
          const double expected = ticks * path.msPerTick;
          const double observed = std::max(0.0, note.onsetMs - path.startMs);
          const double ratio = std::log((observed + kTimingFloorMs) /
                                        (expected + kTimingFloorMs));
          moved.cost += kTimingWeight * ratio * ratio;
          if (ticks >= kMinTempoTicks && observed > 0.0)
          {
            const double measured = std::log(observed / ticks);
            const double msPerTick =
                std::exp((1.0 - kTempoSmoothing) * std::log(path.msPerTick) +
                         kTempoSmoothing * measured);
            moved.msPerTick =
                std::clamp(msPerTick, nominalMsPerTick / kMaxTempoRatio,
                           nominalMsPerTick / kMinTempoRatio);
          }
        }
        moved.event = to;
        moved.startMs = note.onsetMs;
        moved.played = uint64_t{1} << index;
        moved.pending = {path.link, j, to, Step::match, index};
        offer(moved);
      };

      const int last = std::min(from + kMaxAdvance, numEvents - 1);
      for (int to = from + 1; to <= last; ++to)
      {
        const double cost =
            current ? skipCost[to] - skipCost[from + 1]
                    : kLateStart * to; // where the performance starts
        moveTo(to, current ? events[to].tick - current->tick : 0, cost);
      }

      // Over a repeat: on to where the score goes on as written, in a later
      // pass, beyond the moves above.
      if (current)
      {
        const auto written = byWrittenTick.upper_bound(current->writtenTick);
        if (written != byWrittenTick.end())
          for (const int to : written->second)
            if (to > last)
              moveTo(to, written->first - current->writtenTick, kRepeatSkip);
      }
    }

    double cheapest = std::numeric_limits<double>::infinity();
    for (const auto &[event, kept] : next)
      for (const Path &path : kept)
        cheapest = std::min(cheapest, path.cost);
    paths.clear();
    for (auto &[event, kept] : next)
      for (Path &path : kept)
        if (path.cost <= cheapest + kBeam)
        {
          links.push_back(path.pending);
          path.link = static_cast<int>(links.size()) - 1;
          paths.push_back(path);
        }
  }

  // The cheapest path, the score left unplayed after it counted.
  const Path *best = nullptr;
  double finalCost = std::numeric_limits<double>::infinity();
  for (const Path &path : paths)
  {
    const int event = path.event;
    const double cost =
        path.cost + kMissingPitch * CountUnplayed(events[event], path.played) +
        missingCost[events.size()] - missingCost[event + 1];
    if (cost < finalCost)
    {
      finalCost = cost;
      best = &path;
    }
  }

  for (int l = best->link; l >= 0; l = links[l].parent)
  {
    const Link &link = links[l];
    switch (link.step)
    {
    case Step::match:
      alignment.events[link.event].notes[link.pitchIndex] = link.note;
      break;
    case Step::trill:
      alignment.events[TrillStart(events, link.event, notes[link.note].pitch)]
          .trillNotes.push_back(link.note);
      break;
    case Step::extra:
      alignment.added.push_back(link.note);
      break;
    }
  }

  for (auto &event : alignment.events)
  {
    std::sort(event.trillNotes.begin(), event.trillNotes.end());
    std::vector<double> onsets;
    for (const size_t note : event.notes)
      if (note != PerformanceAlignment::none)
        onsets.push_back(notes[note].onsetMs);
    if (!onsets.empty())
    {
      std::sort(onsets.begin(), onsets.end());
      event.ms = onsets[(onsets.size() - 1) / 2];
    }
  }
  std::sort(alignment.added.begin(), alignment.added.end());
  return alignment;
}
} // namespace dgk
