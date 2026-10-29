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
#include "TakeReplayer.h"
#include "MuseScoreShell/OrchestrionActionIds.h"
#include "TakeRecorder.h"

#include <QTimer>
#include <cmath>
#include <notation/imasternotation.h>
#include <notation/inotationplayback.h>

namespace dgk
{
namespace
{
/**
 * How long the settings are given to take effect before the take starts:
 * changing the staff modes realigns the hands, changing the ornament mode
 * makes another sequencer.
 */
constexpr int settleMs = 300;
} // namespace

TakeReplayer::TakeReplayer(TakeRecorder &recorder) : m_recorder{recorder} {}

void TakeReplayer::init()
{
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this]
                                             {
                                               // A take waiting for its
                                               // score to open.
                                               if (m_take && m_index == 0 &&
                                                   orchestrion()->sequencer())
                                                 Start();
                                             });
}

void TakeReplayer::Replay(Take take)
{
  Stop();
  m_take = std::move(take);
  m_index = 0;
  m_recorder.SetPaused(true);
  if (orchestrion()->sequencer())
    Start();
}

void TakeReplayer::Stop()
{
  ++m_generation;
  if (!m_take)
    return;
  m_take.reset();
  m_recorder.SetPaused(false);
}

void TakeReplayer::Start()
{
  const Take &take = *m_take;
  for (const int staff : {0, 1})
    if (staffModes()->mode(staff) != take.staffModes[staff])
      staffModes()->setMode(staff, take.staffModes[staff]);
  if (sequencerConfiguration()->ornamentMode() != take.ornamentMode)
    sequencerConfiguration()->setOrnamentMode(take.ornamentMode);
  if (const auto notation = projectSession()->masterNotation())
  {
    const auto playback = notation->playback();
    if (take.loopInTick != take.loopOutTick)
    {
      playback->addLoopBoundary(mu::notation::LoopBoundaryType::LoopIn,
                                take.loopInTick);
      playback->addLoopBoundary(mu::notation::LoopBoundaryType::LoopOut,
                                take.loopOutTick);
    }
    playback->setLoopBoundariesEnabled(take.loopEnabled);
  }

  const unsigned generation = ++m_generation;
  QTimer::singleShot(settleMs, Qt::PreciseTimer,
                     [this, generation]
                     {
                       if (generation != m_generation || !m_take)
                         return;
                       const auto sequencer = orchestrion()->sequencer();
                       if (!sequencer)
                         return Stop();
                       sequencer->GoToTick(m_take->startTick,
                                           JumpReason::Click);
                       m_clock.start();
                       ScheduleNext();
                     });
}

void TakeReplayer::ScheduleNext()
{
  if (!m_take)
    return;
  if (m_index >= m_take->events.size())
    return Stop();
  // Due on the take's own clock: a timer firing late doesn't delay the rest.
  const double dueMs = m_take->events[m_index].ms;
  const double nowMs = static_cast<double>(m_clock.nsecsElapsed()) / 1e6;
  const int delay = std::max(0, static_cast<int>(std::lround(dueMs - nowMs)));
  const unsigned generation = m_generation;
  QTimer::singleShot(delay, Qt::PreciseTimer,
                     [this, generation]
                     {
                       if (generation == m_generation)
                         Fire();
                     });
}

void TakeReplayer::Fire()
{
  if (!m_take)
    return;
  const Take::Event &e = m_take->events[m_index++];
  switch (e.kind)
  {
  case Take::Event::Kind::noteOn:
    gestureInput()->noteOn().send(e.pitch, e.velocity);
    break;
  case Take::Event::Kind::noteOff:
    gestureInput()->noteOff().send(e.pitch);
    break;
  case Take::Event::Kind::sustainPedal:
    gestureInput()->sustainPedal().send(e.pedalDown);
    break;
  case Take::Event::Kind::playbackStarted:
    if (!orchestrion()->player()->IsPlaying())
      dispatcher()->dispatch(actionIds::playbackToggle);
    break;
  case Take::Event::Kind::playbackStopped:
    if (orchestrion()->player()->IsPlaying())
      dispatcher()->dispatch(actionIds::playbackStop);
    break;
  }
  ScheduleNext();
}
} // namespace dgk
