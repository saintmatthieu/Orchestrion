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
#include "PerformanceEndWatcher.h"

#include "OrchestrionSequencer/IMelodySegment.h"

#include <algorithm>
#include <chrono>

namespace dgk
{
namespace
{
// A moment to take in the end of the piece.
constexpr auto delayAfterEnd = std::chrono::seconds{3};
} // namespace

PerformanceEndWatcher::PerformanceEndWatcher()
{
  m_delay.setSingleShot(true);
  m_delay.setInterval(delayAfterEnd);
  QObject::connect(&m_delay, &QTimer::timeout, [this]
                   { m_reachedEnd.notify(); });
}

void PerformanceEndWatcher::start()
{
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this] { followSequencer(); });
  followSequencer();
}

muse::async::Notification PerformanceEndWatcher::reachedEnd() const
{
  return m_reachedEnd;
}

muse::async::Notification PerformanceEndWatcher::leftEnd() const
{
  return m_leftEnd;
}

void PerformanceEndWatcher::followSequencer()
{
  // Another score: the end reached was that of the previous one.
  m_delay.stop();
  m_atEnd = false;
  m_performerStruck = false;
  m_leftEnd.notify();
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  sequencer->HandNoteEvents().onReceive(
      this,
      [this](const AutoPlayEvent &event)
      {
        if (event.type != NoteEventType::noteOn)
          return;
        onStruck();
      },
      muse::async::Asyncable::Mode::SetReplace);
  sequencer->ChordTransitions().onReceive(
      this,
      [this](const std::map<TrackIndex, ChordTransition> &)
      { onChordTransition(); },
      muse::async::Asyncable::Mode::SetReplace);
}

void PerformanceEndWatcher::onStruck()
{
  // Whose the notes are is told at the strike: the automatic player (a
  // tutorial, a replay) may have stopped by the time its last release gets
  // here.
  m_performerStruck = !orchestrion()->player()->IsPlaying();
  m_delay.stop();
  m_atEnd = false;
  m_leftEnd.notify();
}

void PerformanceEndWatcher::onChordTransition()
{
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  // The keys of a Notes staff aren't hand gestures: they show here.
  const auto &sounding = sequencer->GetCurrentTransitions();
  if (std::any_of(sounding.begin(), sounding.end(),
                  [](const auto &entry)
                  {
                    const auto *const present = GetPresentThing(entry.second);
                    return present && present->AsChord();
                  }))
  {
    onStruck();
    return;
  }
  if (!m_performerStruck || m_atEnd)
    return;
  // The end, as for the final score: the last notes released, nothing is
  // sounding and nothing is to come on any voice.
  const auto &current = sequencer->GetCurrentTransitions();
  const bool atEnd = !current.empty() &&
                     std::all_of(current.begin(), current.end(),
                                 [](const auto &entry) {
                                   return !GetFutureChord(entry.second) &&
                                          !GetPresentThing(entry.second);
                                 });
  if (atEnd)
  {
    m_atEnd = true;
    m_delay.start();
  }
}
} // namespace dgk
