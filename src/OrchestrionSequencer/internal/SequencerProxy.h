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
#pragma once

#include "IOrchestrionSequencer.h"
#include "IOrchestrionSequencerConfiguration.h"
#include "IStaffModes.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "OrchestrionNotation/IOrchestrionNotationInteractionProcessor.h"
#include "PositionEstimation/PositionTracker.h"
#include "internal/OrchestrionSequencer.h"
#include "internal/PlayThread.h"

#include <QObject>
#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <map>
#include <memory>
#include <project/iprojectsession.h>

namespace dgk
{
/**
 * The sequencer as the main thread sees it, the sequencer itself playing on
 * the play thread (see ../threading.md). What is asked of it is posted
 * there. What it tells comes back in the order it was told, and its state,
 * as of what was told last: the current transitions, the tracks' next
 * chords, the tempo estimate. Tells it of the settings of the main thread's
 * that it reads.
 *
 * Created on the main thread, and destroyed there, whatever thread lets go
 * of it last.
 */
class SequencerProxy : public IOrchestrionSequencer,
                       public muse::async::Asyncable,
                       public muse::actions::Actionable,
                       public dgk::Injectable
{
  dgk::Inject<mu::project::IProjectSession> projectSession{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<IOrchestrionNotationInteractionProcessor> interactionProcessor{
      this};
  dgk::Inject<IOrchestrionSequencerConfiguration> configuration{this};
  dgk::Inject<IStaffModes> staffModes{this};

public:
  /** `sequencer` mustn't have been used on the play thread yet. */
  SequencerProxy(std::shared_ptr<OrchestrionSequencer> sequencer,
                 std::shared_ptr<PlayThread>);
  ~SequencerProxy() override;

  void OnInputEvent(NoteEventType, int pitch,
                    std::optional<float> velocity) override;
  void OnHandInputEvent(NoteEventType, bool isLeftHand,
                        std::optional<float> velocity) override;
  void OnSustainPedal(bool down) override;
  void SetFootPedalling(bool) override;
  void AllNotesOff() override;

  std::map<TrackIndex, ChordTransition> GetCurrentTransitions() const override;
  std::vector<TrackIndex> GetAllVoices() const override;
  const IChord *GetNextChord(TrackIndex) const override;
  muse::async::Channel<std::map<TrackIndex, ChordTransition>>
  ChordTransitions() const override;
  muse::async::Channel<AutoPlayEvent> HandNoteEvents() const override;
  muse::async::Channel<EventVariant> OutputEvent() const override;
  muse::async::Channel<EventVariant> MainThreadOutputEvent() const override;
  muse::async::Channel<std::vector<const IChord *>> WrongNotes() const override;
  muse::async::Channel<int, JumpReason> AboutToJumpPosition() const override;
  muse::async::Channel<IGestureInput::Gesture> Gestures() const override;
  std::chrono::steady_clock::time_point EventTime() const override;
  void GoToTick(int tick, JumpReason reason) override;
  void GoToEnd() override;
  std::optional<PerformedTempo> PerformedTempoNow() override;
  void SkipHandTo(bool isLeftHand, int tickWithRepeats) override;
  void GoToPrevNoteonTick() override;
  void GoToNextNoteonTick() override;
  /** Waits for the play thread: meant for the automatic player, there. */
  std::optional<NextAutoPlayEvents> WhatToPlayNext() override;
  KeyRole RoleOfKey(int pitch) const override;

private:
  /** Runs `task` on the sequencer, on the play thread. */
  void Post(std::function<void(OrchestrionSequencer &)> task);
  /**
   * On the sequencer's threads: runs `task` on the main thread, after what
   * was relayed before (from the same thread), EventTime() telling when it
   * was relayed.
   */
  void Relay(std::function<void()> task);
  /** On the play thread: each track's next chord. */
  std::map<TrackIndex, const IChord *> NextChords() const;
  void PostStaffModes();

  const std::shared_ptr<OrchestrionSequencer> m_sequencer;
  const std::shared_ptr<PlayThread> m_thread;
  /** Its subscriptions to the sequencer, made and let go of there. */
  std::unique_ptr<muse::async::Asyncable> m_playThreadSubscriptions;
  /**
   * What the relayed tasks run in, on the main thread: they are dropped with
   * it.
   */
  QObject m_mainThreadContext;

  // As of what was relayed last
  std::chrono::steady_clock::time_point m_eventTime;
  std::map<TrackIndex, ChordTransition> m_transitions;
  std::map<TrackIndex, const IChord *> m_nextChords;
  /** None until told: no estimate yet. */
  std::optional<PositionTracker> m_tempo;

  muse::async::Channel<std::map<TrackIndex, ChordTransition>>
      m_transitionsChannel;
  muse::async::Channel<AutoPlayEvent> m_handNoteEvents;
  muse::async::Channel<std::vector<const IChord *>> m_wrongNotes;
  muse::async::Channel<int, JumpReason> m_aboutToJumpPosition;
  muse::async::Channel<EventVariant> m_mainThreadOutputEvent;
  muse::async::Channel<IGestureInput::Gesture> m_gestures;
};
} // namespace dgk
