/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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
#include "OrchestrionNotation/IOrchestrionNotationInteractionProcessor.h"
#include "OrchestrionTypes.h"
#include "internal/VoiceSequencer.h"
#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <array>
#include <async/asyncable.h>
#include <chrono>
#include <context/iglobalcontext.h>
#include <functional>
#include <memory>
#include <modularity/ioc.h>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <vector>

namespace dgk
{
class OrchestrionSequencer : public IOrchestrionSequencer,
                             public muse::Injectable,
                             public muse::async::Asyncable,
                             public muse::actions::Actionable
{
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;
  muse::Inject<IOrchestrionNotationInteractionProcessor> interactionProcessor;

public:
  using HandVoices = std::vector<std::unique_ptr<VoiceSequencer>>;

  OrchestrionSequencer(InstrumentIndex, HandVoices rightHand,
                       HandVoices leftHand, PedalSequence);
  ~OrchestrionSequencer();

  void OnInputEvent(NoteEventType, int pitch, float velocity) override;

  const std::map<TrackIndex, ChordTransition> &
  GetCurrentTransitions() const override;
  std::vector<TrackIndex> GetAllVoices() const override;
  muse::async::Channel<std::map<TrackIndex, ChordTransition>>
  ChordTransitions() const override;
  muse::async::Channel<EventVariant> OutputEvent() const override;
  muse::async::Notification AboutToJumpPosition() const override;

private:
  struct Hand
  {
    HandVoices voices;
    std::optional<int> pressedKey;
  };

  void SendTransitions(std::map<TrackIndex, ChordTransition>,
                       float velocity = 0.f);
  void GoToTick(int tick);
  std::map<TrackIndex, ChordTransition> PrepareStaffransitions(
      const HandVoices &,
      std::function<std::optional<ChordTransition>(VoiceSequencer &)>);

  using OptTimePoint =
      std::optional<std::chrono::time_point<std::chrono::steady_clock>>;

  template <typename EventType> struct QueueEntry
  {
    OptTimePoint time;
    EventType event;
  };

  template <typename EventType> struct ThreadMembers
  {
    std::queue<QueueEntry<EventType>> queue;
    std::mutex mutex;
    std::condition_variable cv;
  };

  template <typename EventType>
  static std::thread MakeThread(OrchestrionSequencer &self,
                                ThreadMembers<EventType> &members,
                                std::function<void(EventType)> cb);

  void OnInputEventRecursive(NoteEventType, int pitch, float velocity,
                             bool loop);
  void PostPedalEvent(PedalEvent event);
  void PostNoteEvents(NoteEvents events);

  const InstrumentIndex m_instrument;

  Hand m_rightHand;
  Hand m_leftHand;
  const std::vector<const VoiceSequencer *> m_allVoices;
  const Tick m_finalTick;
  const PedalSequence m_pedalSequence;
  PedalSequence::const_iterator m_pedalSequenceIt;

  std::thread m_pedalThread;
  ThreadMembers<PedalEvent> m_pedalThreadMembers;
  std::thread m_noteThread;
  ThreadMembers<NoteEvent> m_noteThreadMembers;

  bool m_finished = false;
  bool m_pedalDown = false;
  std::mt19937 m_rng{0};
  std::uniform_int_distribution<int> m_delayDist{0, 30000};   // microseconds
  std::uniform_int_distribution<int> m_velocityDist{90, 110}; // percents

  muse::ValCh<std::map<TrackIndex, ChordTransition>> m_transitions;
  muse::async::Channel<EventVariant> m_outputEvent;
  muse::async::Notification m_aboutToJumpPosition;
};
} // namespace dgk
