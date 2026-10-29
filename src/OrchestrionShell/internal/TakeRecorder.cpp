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
#include "TakeRecorder.h"
#include "OrchestrionSequencer/IChord.h"

#include <engraving/dom/masterscore.h>
#include <engraving/dom/measure.h>
#include <notation/imasternotation.h>
#include <notation/inotationplayback.h>
#include <project/inotationproject.h>

namespace dgk
{
void TakeRecorder::init()
{
  gestureInput()->noteOn().onReceive(
      this,
      [this](int pitch, std::optional<float> velocity)
      {
        Take::Event e;
        e.kind = Take::Event::Kind::noteOn;
        e.pitch = pitch;
        e.velocity = velocity;
        Add(e);
      });
  gestureInput()->noteOff().onReceive(this,
                                      [this](int pitch)
                                      {
                                        Take::Event e;
                                        e.kind = Take::Event::Kind::noteOff;
                                        e.pitch = pitch;
                                        Add(e);
                                      });
  gestureInput()->sustainPedal().onReceive(
      this,
      [this](bool down)
      {
        Take::Event e;
        e.kind = Take::Event::Kind::sustainPedal;
        e.pedalDown = down;
        Add(e);
      });
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this] { OnSequencerChanged(); });
  sequencerConfiguration()->ornamentModeChanged().onNotify(
      this, [this] { EndTake(std::nullopt); });
  projectSession()->masterNotationChanged().onNotify(this, [this]
                                                     { OnNotationChanged(); });
  OnSequencerChanged();
  OnNotationChanged();
}

void TakeRecorder::OnSequencerChanged()
{
  EndTake(std::nullopt);
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  // Every jump ends the take: those the performer makes, and the realigning
  // a change of the staff modes makes. A loop's wrap and the score's end are
  // part of the playing.
  sequencer->AboutToJumpPosition().onReceive(
      this,
      [this](int tick, JumpReason reason)
      {
        if (reason != JumpReason::LoopWrap && reason != JumpReason::ScoreEnd)
          EndTake(tick);
      },
      muse::async::Asyncable::Mode::SetReplace);
  orchestrion()->player()->PlayingChanged().onNotify(
      this,
      [this]
      {
        Take::Event e;
        e.kind = orchestrion()->player()->IsPlaying()
                     ? Take::Event::Kind::playbackStarted
                     : Take::Event::Kind::playbackStopped;
        // Playback alone is no take: only within one already under way.
        if (m_take)
          Add(e);
      },
      muse::async::Asyncable::Mode::SetReplace);
}

void TakeRecorder::OnNotationChanged()
{
  EndTake(std::nullopt);
  if (const auto notation = projectSession()->masterNotation())
    notation->playback()->loopBoundariesChanged().onNotify(
        this, [this] { EndTake(std::nullopt); },
        muse::async::Asyncable::Mode::SetReplace);
}

std::vector<Take> TakeRecorder::Takes() const
{
  std::vector<Take> takes = m_takes;
  if (m_take && m_take->NoteCount() > 0)
    takes.push_back(*m_take);
  return takes;
}

void TakeRecorder::Clear()
{
  m_takes.clear();
  m_take.reset();
}

void TakeRecorder::SetPaused(bool paused)
{
  EndTake(std::nullopt);
  m_paused = paused;
}

bool TakeRecorder::Recording() const
{
  return !m_paused && sequencerConfiguration()->takeRecordingEnabled() &&
         orchestrion()->sequencer();
}

void TakeRecorder::EndTake(std::optional<int> tick)
{
  if (m_take && m_take->NoteCount() > 0)
    m_takes.push_back(std::move(*m_take));
  m_take.reset();
  m_nextStartTick = tick;
}

void TakeRecorder::Add(Take::Event e)
{
  if (!Recording())
    return;
  if (!m_take)
  {
    if (e.kind != Take::Event::Kind::noteOn)
      // A release or a pedal from before: no take begins with it.
      return;
    Take take;
    const auto notation = projectSession()->masterNotation();
    if (const auto project = projectSession()->project())
      take.scorePath = project->path().toQString();
    take.startTick =
        m_nextStartTick ? *m_nextStartTick : NextChordTick().value_or(0);
    take.startBar = BarAt(take.startTick);
    take.staffModes = {staffModes()->mode(0), staffModes()->mode(1)};
    take.ornamentMode = sequencerConfiguration()->ornamentMode();
    if (notation)
    {
      const auto &loop = notation->playback()->loopBoundaries();
      take.loopEnabled = loop.enabled;
      take.loopInTick = loop.loopInTick.ticks();
      take.loopOutTick = loop.loopOutTick.ticks();
    }
    m_take = std::move(take);
    m_clock.start();
  }
  e.ms = static_cast<double>(m_clock.nsecsElapsed()) / 1e6;
  m_take->events.push_back(e);
}

std::optional<int> TakeRecorder::NextChordTick() const
{
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return std::nullopt;
  std::optional<int> tick;
  for (const TrackIndex &track : sequencer->GetAllVoices())
    if (const IChord *chord = sequencer->GetNextChord(track))
    {
      const int t = chord->GetBeginTick().withoutRepeats;
      tick = tick ? std::min(*tick, t) : t;
    }
  return tick;
}

int TakeRecorder::BarAt(int tick) const
{
  const auto notation = projectSession()->masterNotation();
  if (!notation)
    return 1;
  const mu::engraving::Measure *measure = notation->masterScore()->tick2measure(
      mu::engraving::Fraction::fromTicks(tick));
  return measure ? measure->measureIndex() + 1 : 1;
}
} // namespace dgk
