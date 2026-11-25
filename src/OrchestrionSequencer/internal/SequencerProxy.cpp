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
#include "SequencerProxy.h"

#include <QMetaObject>
#include <QThread>
#include <engraving/dom/engravingitem.h>
#include <notation/inotationplayback.h>

namespace dgk
{
SequencerProxy::SequencerProxy(std::shared_ptr<OrchestrionSequencer> sequencer,
                               std::shared_ptr<PlayThread> thread)
    : m_sequencer{std::move(sequencer)}, m_thread{std::move(thread)},
      m_transitions{m_sequencer->GetCurrentTransitions()},
      m_nextChords{NextChords()}
{
  // What it tells, on the play thread, relayed to the main thread
  m_thread->Wait(
      [this]
      {
        m_playThreadSubscriptions = std::make_unique<muse::async::Asyncable>();
        auto *const subscriber = m_playThreadSubscriptions.get();
        OrchestrionSequencer &sequencer = *m_sequencer;
        sequencer.ChordTransitions().onReceive(
            subscriber,
            [this](const std::map<TrackIndex, ChordTransition> &transitions)
            {
              Relay(
                  [this, transitions, next = NextChords()]
                  {
                    m_transitions = transitions;
                    m_nextChords = next;
                    m_transitionsChannel.send(transitions);
                  });
            });
        sequencer.AboutToJumpPosition().onReceive(
            subscriber,
            [this](int tick, JumpReason reason) {
              Relay([this, tick, reason]
                    { m_aboutToJumpPosition.send(tick, reason); });
            });
        sequencer.HandNoteEvents().onReceive(
            subscriber, [this](const AutoPlayEvent &event)
            { Relay([this, event] { m_handNoteEvents.send(event); }); });
        sequencer.Gestures().onReceive(
            subscriber, [this](const IGestureInput::Gesture &gesture)
            { Relay([this, gesture] { m_gestures.send(gesture); }); });
        sequencer.WrongNotes().onReceive(
            subscriber, [this](const std::vector<const IChord *> &chords)
            { Relay([this, chords] { m_wrongNotes.send(chords); }); });
        sequencer.TempoChanged().onReceive(
            subscriber,
            [this](const std::shared_ptr<const PositionTracker> &tempo)
            { Relay([this, tempo] { m_tempo.emplace(*tempo); }); });
      });

  // What is to sound, sent from the sequencer's threads, relayed too
  m_sequencer->SetOutputObserver(
      [this](const EventVariant &event)
      { Relay([this, event] { m_mainThreadOutputEvent.send(event); }); });
  m_sequencer->SetMainThreadPoster([this](std::function<void()> task)
                                   { Relay(std::move(task)); });

  // The main thread's settings, told to the play thread
  staffModes()->modesChanged().onNotify(this, [this] { PostStaffModes(); });
  configuration()->velocityRecordingEnabledChanged().onNotify(
      this,
      [this]
      {
        Post([enabled = configuration()->velocityRecordingEnabled()](
                 OrchestrionSequencer &sequencer)
             { sequencer.SetVelocityRecordingEnabled(enabled); });
      });
  configuration()->ornamentModeChanged().onNotify(
      this,
      [this]
      {
        Post(
            [enabled = configuration()->ornamentMode() ==
                       OrnamentMode::automatic](OrchestrionSequencer &sequencer)
            { sequencer.SetOrnamentsEnabled(enabled); });
      });
  if (const auto *const notation = m_sequencer->Notation())
    if (const auto playback = notation->playback())
      playback->loopBoundariesChanged().onNotify(
          this,
          [this, playback]
          {
            Post([loop = playback->loopBoundaries()](
                     OrchestrionSequencer &sequencer)
                 { sequencer.SetLoopBoundaries(loop); });
          });

  dispatcher()->reg(this, "nav-first-control",
                    [this] { GoToTick(0, JumpReason::Rewind); });
  interactionProcessor()->itemClicked().onReceive(
      this, [this](const mu::engraving::EngravingItem *item)
      { GoToTick(item->tick().ticks(), JumpReason::Click); });
}

SequencerProxy::~SequencerProxy()
{
  // Nothing relayed any more: the subscriptions go where they were made.
  // What was relayed and not run yet goes with m_mainThreadContext.
  m_thread->Wait([this] { m_playThreadSubscriptions.reset(); });
  m_sequencer->SetOutputObserver(nullptr);
  m_sequencer->SetMainThreadPoster(nullptr);
}

void SequencerProxy::Post(std::function<void(OrchestrionSequencer &)> task)
{
  m_thread->Post([sequencer = m_sequencer, task = std::move(task)]
                 { task(*sequencer); });
}

void SequencerProxy::Relay(std::function<void()> task)
{
  const auto time = std::chrono::steady_clock::now();
  if (QThread::currentThread() == m_mainThreadContext.thread())
  {
    // Offline rendering: the play thread is the main thread.
    m_eventTime = time;
    task();
  }
  else
    QMetaObject::invokeMethod(
        &m_mainThreadContext,
        [this, time, task = std::move(task)]
        {
          m_eventTime = time;
          task();
        },
        Qt::QueuedConnection);
}

std::chrono::steady_clock::time_point SequencerProxy::EventTime() const
{
  return m_eventTime;
}

std::map<TrackIndex, const IChord *> SequencerProxy::NextChords() const
{
  std::map<TrackIndex, const IChord *> next;
  for (const TrackIndex &track : m_sequencer->GetAllVoices())
    next.emplace(track, m_sequencer->GetNextChord(track));
  return next;
}

void SequencerProxy::PostStaffModes()
{
  if (projectSession()->masterNotation().get() != m_sequencer->Notation())
    // Another score's: this sequencer is on its way out.
    return;
  Post([modes = OrchestrionSequencer::Modes{staffModes()->mode(0),
                                            staffModes()->mode(1)}](
           OrchestrionSequencer &sequencer)
       { sequencer.SetStaffModes(modes); });
}

std::map<TrackIndex, ChordTransition>
SequencerProxy::GetCurrentTransitions() const
{
  return m_transitions;
}

std::vector<TrackIndex> SequencerProxy::GetAllVoices() const
{
  // Never changes
  return m_sequencer->GetAllVoices();
}

const IChord *SequencerProxy::GetNextChord(TrackIndex track) const
{
  const auto it = m_nextChords.find(track);
  return it == m_nextChords.end() ? nullptr : it->second;
}

muse::async::Channel<std::map<TrackIndex, ChordTransition>>
SequencerProxy::ChordTransitions() const
{
  return m_transitionsChannel;
}

muse::async::Channel<AutoPlayEvent> SequencerProxy::HandNoteEvents() const
{
  return m_handNoteEvents;
}

muse::async::Channel<IGestureInput::Gesture> SequencerProxy::Gestures() const
{
  return m_gestures;
}

muse::async::Channel<EventVariant> SequencerProxy::MainThreadOutputEvent() const
{
  return m_mainThreadOutputEvent;
}

muse::async::Channel<std::vector<const IChord *>>
SequencerProxy::WrongNotes() const
{
  return m_wrongNotes;
}

muse::async::Channel<int, JumpReason>
SequencerProxy::AboutToJumpPosition() const
{
  return m_aboutToJumpPosition;
}

void SequencerProxy::GoToTick(int tick, JumpReason reason)
{
  Post([=](OrchestrionSequencer &sequencer)
       { sequencer.GoToTick(tick, reason); });
}

void SequencerProxy::GoToEnd()
{
  Post([](OrchestrionSequencer &sequencer) { sequencer.GoToEnd(); });
}

std::optional<PerformedTempo> SequencerProxy::PerformedTempoNow()
{
  // As OrchestrionSequencer::PerformedTempoNow, on the copy
  const double nowMs = m_sequencer->NowMs();
  if (!m_tempo)
    return std::nullopt;
  m_tempo->heartbeat(nowMs);
  if (!m_tempo->ready())
    return std::nullopt;
  return PerformedTempo{m_tempo->positionAt(nowMs), m_tempo->speed(),
                        m_tempo->isCoasting()};
}

void SequencerProxy::GoToPrevNoteonTick()
{
  Post([](OrchestrionSequencer &sequencer) { sequencer.GoToPrevNoteonTick(); });
}

void SequencerProxy::GoToNextNoteonTick()
{
  Post([](OrchestrionSequencer &sequencer) { sequencer.GoToNextNoteonTick(); });
}

IOrchestrionSequencer::KeyRole SequencerProxy::RoleOfKey(int pitch) const
{
  return m_sequencer->RoleOfKey(
      OrchestrionSequencer::Modes{staffModes()->mode(0), staffModes()->mode(1)},
      pitch);
}

std::shared_ptr<const RecordingTimeline>
SequencerProxy::PlayPerformance(std::vector<PerformedNote> notes)
{
  // On the main thread: the score's events don't change.
  PerformancePlayback playback;
  if (!notes.empty())
  {
    std::sort(notes.begin(), notes.end(),
              [](const PerformedNote &a, const PerformedNote &b)
              { return a.onsetMs < b.onsetMs; });
    const std::vector<ScoreEvent> &events = m_sequencer->ScoreEvents();
    playback = MakePerformancePlayback(
        events, notes,
        AlignPerformance(events, notes, m_sequencer->NominalTicksPerMs()));
  }
  Post([velocities = playback.velocities](OrchestrionSequencer &sequencer)
       { sequencer.SetPerformedVelocities(velocities); });
  return playback.timeline;
}
} // namespace dgk
