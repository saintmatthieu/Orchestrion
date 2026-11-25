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

namespace dgk
{
namespace
{
std::int64_t Key(int tick, int pitch)
{
  return (static_cast<std::int64_t>(tick) << 8) | (pitch & 0xff);
}

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

std::optional<float> PerformedVelocities::At(int tick, int pitch) const
{
  const auto it = m_velocities.find(Key(tick, pitch));
  if (it == m_velocities.end())
    return std::nullopt;
  return it->second;
}

void PerformedVelocities::Set(int tick, int pitch, float velocity)
{
  m_velocities[Key(tick, pitch)] = velocity;
}

PerformancePlayback
MakePerformancePlayback(const std::vector<ScoreEvent> &events,
                        const std::vector<PerformedNote> &notes,
                        const PerformanceAlignment &alignment)
{
  PerformancePlayback playback;
  if (alignment.events.size() != events.size())
    return playback;

  auto velocities = std::make_shared<PerformedVelocities>();
  std::vector<RecordingBeat> beats;
  std::optional<size_t> last; // the last event timed
  for (size_t i = 0; i < events.size(); ++i)
  {
    const ScoreEvent &event = events[i];
    const PerformanceAlignment::Event &performed = alignment.events[i];
    for (size_t p = 0; p < performed.notes.size() && p < event.pitches.size();
         ++p)
      if (performed.notes[p] < notes.size())
        velocities->Set(
            event.tick, event.pitches[p],
            std::clamp(notes[performed.notes[p]].velocity / 127.f, 0.f, 1.f));

    // Times in order only: one at or before the last is the alignment's
    // doubt, and the timeline wants them rising.
    if (!performed.ms ||
        (!beats.empty() && *performed.ms / 1000.0 <= beats.back().seconds))
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
                     *performed.ms / 1000.0, afterSkip});
    last = i;
  }

  if (!velocities->Empty())
    playback.velocities = std::move(velocities);
  if (!beats.empty())
  {
    auto timeline = std::make_shared<const RecordingTimeline>(
        std::move(beats), Stretches(events));
    if (!timeline->Empty())
      playback.timeline = std::move(timeline);
  }
  return playback;
}
} // namespace dgk
