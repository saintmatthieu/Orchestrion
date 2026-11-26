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
#include "PerformancePlayback.h"

#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
/** Notes struck this close together are one step of an ornament. */
constexpr double kTogetherMs = 20.0;

/**
 * The score's stretches played straight through, by their events: where
 * the tick with repeats and the tick as written keep the same distance.
 * Each reaches the next one's start, the last its last event and a beat.
 */
std::vector<RecordingBar> Stretches(const std::vector<ScoreEvent> &events)
{
  std::vector<RecordingBar> stretches;
  for (size_t i = 0; i < events.size(); ++i)
  {
    const ScoreEvent &event = events[i];
    if (!stretches.empty() &&
        event.tick - event.writtenTick ==
            stretches.back().tick - stretches.back().writtenTick)
      continue;
    if (!stretches.empty())
      stretches.back().length = event.tick - stretches.back().tick;
    stretches.push_back({event.tick, event.writtenTick, 0});
  }
  if (!stretches.empty())
    stretches.back().length = events.back().tick - stretches.back().tick + 1;
  return stretches;
}
} // namespace

struct PerformanceBuilder
{
  static PerformancePlayback Make(const std::vector<ScoreEvent> &events,
                                  const std::vector<PerformedNote> &notes,
                                  const PerformanceAlignment &alignment);
};

std::optional<float> PerformedScore::Velocity(int tick, int pitch) const
{
  const auto event = m_events.find(tick);
  if (event == m_events.end())
    return std::nullopt;
  for (const PerformedNote &note : event->second.struck)
    if (note.pitch == pitch)
      return std::clamp(note.velocity / 127.f, 0.f, 1.f);
  return std::nullopt;
}

std::vector<PerformedNote>
PerformedScore::NotesBetween(int begin, int end,
                             const std::set<int> &pitches) const
{
  std::vector<PerformedNote> notes;
  for (auto event = m_events.lower_bound(begin);
       event != m_events.end() && event->first < end; ++event)
  {
    for (const PerformedNote &note : event->second.struck)
      if (pitches.count(note.pitch))
        notes.push_back(note);
    for (const PerformedNote &note : event->second.trill)
      if (pitches.count(note.pitch))
        notes.push_back(note);
  }
  std::sort(notes.begin(), notes.end(),
            [](const PerformedNote &a, const PerformedNote &b)
            { return a.onsetMs < b.onsetMs; });
  return notes;
}

std::optional<double> PerformedScore::Onset(int tick, int pitch) const
{
  const auto event = m_events.find(tick);
  if (event == m_events.end())
    return std::nullopt;
  for (const PerformedNote &note : event->second.struck)
    if (note.pitch == pitch)
      return note.onsetMs;
  return std::nullopt;
}

std::optional<double> PerformedScore::StrikeMs(int tick) const
{
  const auto event = m_events.find(tick);
  return event == m_events.end() ? std::nullopt : event->second.ms;
}

std::optional<int>
PerformedScore::ReleaseTick(int begin, int end, int minTick,
                            const std::set<int> &pitches) const
{
  const std::vector<PerformedNote> notes = NotesBetween(begin, end, pitches);
  if (notes.empty() || !m_timeline)
    return std::nullopt;
  double offMs = notes.front().offsetMs;
  for (const PerformedNote &note : notes)
    offMs = std::max(offMs, note.offsetMs);
  const auto position = m_timeline->PositionAt(offMs / 1000.0);
  if (!position)
    return std::nullopt;
  return std::clamp(position->tick, std::min(minTick, end), end);
}

std::optional<double> PerformedScore::TicksPerMsAt(int tick) const
{
  const auto secondsPerTick =
      m_timeline ? m_timeline->SecondsPerTick(tick) : std::nullopt;
  if (!secondsPerTick || *secondsPerTick <= 0.0)
    return std::nullopt;
  return 1.0 / (*secondsPerTick * 1000.0);
}

PerformancePlayback
PerformanceBuilder::Make(const std::vector<ScoreEvent> &events,
                         const std::vector<PerformedNote> &notes,
                         const PerformanceAlignment &alignment)
{
  PerformancePlayback playback;
  if (alignment.events.size() != events.size())
    return playback;

  auto performed = std::make_shared<PerformedScore>();
  std::vector<RecordingBeat> beats;
  std::optional<size_t> last; // the last event timed
  for (size_t i = 0; i < events.size(); ++i)
  {
    const ScoreEvent &event = events[i];
    const PerformanceAlignment::Event &aligned = alignment.events[i];
    PerformedScore::Event struck;
    for (const size_t n : aligned.notes)
      if (n < notes.size())
        struck.struck.push_back(notes[n]);
    for (const size_t n : aligned.trillNotes)
      if (n < notes.size())
        struck.trill.push_back(notes[n]);
    // Struck with its earliest note.
    struck.ms = aligned.ms;
    for (const PerformedNote &note : struck.struck)
      struck.ms = std::min(struck.ms.value_or(note.onsetMs), note.onsetMs);
    const std::optional<double> strikeMs = struck.ms;
    if (!struck.struck.empty() || !struck.trill.empty())
      performed->m_events[event.tick] = std::move(struck);

    // Times in order only: one at or before the last is the alignment's
    // doubt, and the timeline wants them rising.
    if (!strikeMs ||
        (!beats.empty() && *strikeMs / 1000.0 <= beats.back().seconds))
      continue;
    const int next = i + 1 < events.size() ? events[i + 1].tick : event.tick;
    const int beatTicks =
        next > event.tick
            ? next - event.tick
            : (beats.empty() ? 1 : event.tick - beats.back().tick);
    // Skipped: further on as played than as written, though going on in
    // the score as written, with nothing played in between (a repeat).
    bool afterSkip = false;
    if (last && *last + 1 < i)
    {
      const ScoreEvent &before = events[*last];
      const int written = event.writtenTick - before.writtenTick;
      afterSkip = written > 0 && event.tick - before.tick > written;
    }
    beats.push_back({event.tick, event.writtenTick, beatTicks,
                     *strikeMs / 1000.0, afterSkip});
    last = i;
  }

  if (!beats.empty())
  {
    auto timeline = std::make_shared<const RecordingTimeline>(
        std::move(beats), Stretches(events));
    if (!timeline->Empty())
      playback.timeline = std::move(timeline);
  }
  if (!performed->m_events.empty())
  {
    performed->m_timeline = playback.timeline;
    playback.performed = std::move(performed);
  }
  return playback;
}

PerformancePlayback
MakePerformancePlayback(const std::vector<ScoreEvent> &events,
                        const std::vector<PerformedNote> &notes,
                        const PerformanceAlignment &alignment)
{
  return PerformanceBuilder::Make(events, notes, alignment);
}

std::optional<OrnamentSchedule>
PerformedOrnament(const std::vector<PerformedNote> &notes, double strikeMs,
                  double msScale, float reference)
{
  if (reference <= 0.f || msScale <= 0.0)
    return std::nullopt;
  struct Step
  {
    double onsetMs = 0.0;
    std::vector<int> pitches;
    int velocity = 0;
  };
  std::vector<Step> steps;
  for (const PerformedNote &note : notes)
  {
    if (steps.empty() || note.onsetMs - steps.back().onsetMs > kTogetherMs)
      steps.push_back({note.onsetMs, {}, 0});
    Step &step = steps.back();
    if (std::find(step.pitches.begin(), step.pitches.end(), note.pitch) ==
        step.pitches.end())
      step.pitches.push_back(note.pitch);
    step.velocity = std::max(step.velocity, note.velocity);
  }
  // The step under way at the strike, or the first if it comes later.
  size_t first = 0;
  while (first + 1 < steps.size() && steps[first + 1].onsetMs <= strikeMs)
    ++first;
  if (steps.size() - first < 2)
    return std::nullopt;

  OrnamentSchedule schedule;
  for (size_t i = first; i < steps.size(); ++i)
  {
    TimedOrnamentStep step;
    step.pitches = steps[i].pitches;
    std::sort(step.pitches.begin(), step.pitches.end());
    // From the strike, the first; then from its own start.
    const double start = i == first ? strikeMs : steps[i].onsetMs;
    if (i + 1 < steps.size())
      step.duration = std::chrono::microseconds{
          std::llround((steps[i + 1].onsetMs - start) * msScale * 1000.0)};
    step.gain = steps[i].velocity / 127.f / reference;
    schedule.steps.push_back(std::move(step));
  }
  return schedule;
}
} // namespace dgk
