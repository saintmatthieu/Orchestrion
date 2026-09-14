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
#include "IOrchestrionSequencerConfiguration.h"
#include "OrchestrionNotation/IOrchestrionNotationInteractionProcessor.h"
#include "OrchestrionTypes.h"
#include "PositionEstimation/PositionTracker.h"
#include "internal/OrnamentSchedule.h"
#include "internal/VoiceSequencer.h"

#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <array>
#include <async/asyncable.h>
#include <chrono>
#include <condition_variable>
#include <context/iglobalcontext.h>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <modularity/ioc.h>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
class OrchestrionSequencer : public IOrchestrionSequencer,
                             public dgk::Injectable,
                             public muse::async::Asyncable,
                             public muse::actions::Actionable
{
  dgk::Inject<mu::context::IGlobalContext> globalContext{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<IOrchestrionNotationInteractionProcessor> interactionProcessor{
      this};
  dgk::Inject<IOrchestrionSequencerConfiguration> configuration{this};

public:
  using HandVoices = std::vector<std::unique_ptr<VoiceSequencer>>;

  OrchestrionSequencer(InstrumentIndex, HandVoices rightHand,
                       HandVoices leftHand, PedalSequence);
  ~OrchestrionSequencer();

  void OnInputEvent(NoteEventType, int pitch,
                    std::optional<float> velocity) override;
  void AllNotesOff() override;

  const std::map<TrackIndex, ChordTransition> &
  GetCurrentTransitions() const override;
  std::vector<TrackIndex> GetAllVoices() const override;
  muse::async::Channel<std::map<TrackIndex, ChordTransition>>
  ChordTransitions() const override;
  muse::async::Channel<AutoPlayEvent> HandNoteEvents() const override;
  muse::async::Channel<EventVariant> OutputEvent() const override;
  muse::async::Channel<int, JumpReason> AboutToJumpPosition() const override;
  void GoToTick(int tick, JumpReason reason) override;
  void GoToPrevNoteonTick() override;
  void GoToNextNoteonTick() override;
  std::optional<NextAutoPlayEvents> WhatToPlayNext() override;

private:
  struct Hand
  {
    HandVoices voices;
    std::optional<int> pressedKey;
    /**
     * The hand's tempo, estimated from the onsets it plays (ticks with
     * repeats against the steady clock), for timing the ornaments it
     * strikes. Reset on a jump, and started over after a pause.
     */
    PositionTracker tempo{};
  };

  /**
   * What a track has sounding and is to sound: the schedule of the chord
   * struck on it, where it has got to, and the move a gesture asked for
   * while a timed step was under way, to be made when the step ends.
   */
  struct TrackSchedule
  {
    OrnamentSchedule schedule;
    /** The step to sound next. */
    size_t next = 0;
    /** The pitches on — those a move silences. */
    std::vector<int> sounding;
    float velocity = 0.f;
    /** Tells this schedule's queue entry from a superseded one's. */
    unsigned generation = 0;
    /** When the timed step under way ends; empty while a step is held. */
    std::optional<std::chrono::steady_clock::time_point> stepEnd;
    struct Move
    {
      /** What to move on to; empty for a release. */
      std::optional<OrnamentSchedule> strike;
      float velocity = 0.f;
      /** Wait for the closing steps to run out, not for the step to end. */
      bool afterClosing = false;
    };
    std::optional<Move> move;
  };

  /** The ornament thread's queue entry: a step's end falling due on a track. */
  struct OrnamentStepDue
  {
    TrackIndex track;
    unsigned generation;
    std::chrono::steady_clock::time_point time;
  };

  void SendTransitions(std::map<TrackIndex, ChordTransition>,
                       std::optional<float> velocity = std::nullopt,
                       bool isLeftHand = false);
  std::map<TrackIndex, ChordTransition> PrepareStaffTransitions(
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
    std::deque<QueueEntry<EventType>> queue;
    std::mutex mutex;
    std::condition_variable cv;
  };

  template <typename EventType>
  static std::thread MakeThread(OrchestrionSequencer &self,
                                ThreadMembers<EventType> &members,
                                std::function<void(EventType)> cb);

  void OnInputEventRecursive(NoteEventType, int pitch,
                             std::optional<float> velocity, bool loop);
  /**
   * Puts the pedal where the hands want it: down when the hands that are down
   * all stand within one and the same pedal span, up when they stand in
   * different spans or in none, unchanged when no hand is down.
   */
  void UpdatePedal();
  /**
   * Queues the pedal event for the pedal thread. A press waits until the pedal
   * has been up for the dampers to act, and a lift supersedes a pending press.
   */
  void PostPedalEvent(PedalEvent event);
  /**
   * Sends the events, several simultaneous strikes slightly spread and
   * shaded like a human's unless `strum` is false.
   */
  void PostNoteEvents(NoteEvents events, bool strum = true);

  /**
   * Feeds the hand's tempo estimate the onset of the chords struck in this
   * batch of transitions, if any.
   */
  void ObserveOnset(Hand &, const std::map<TrackIndex, ChordTransition> &);
  /**
   * The tempo to time the chord's ornament with, in ticks per millisecond:
   * the hand's estimate once it has one, the score's tempo until then.
   */
  double TicksPerMs(const Hand &, const IChord &) const;
  /** How the chord is to sound, struck now at the hand's tempo. */
  OrnamentSchedule ScheduleChord(const IChord &, const Hand &) const;
  /**
   * How far ahead of `strike` the hand is to come down on the chords due
   * there, in ticks, for the grace notes before them to be over by then: the
   * graces' length at the hand's tempo (see OrnamentSchedule::anticipation),
   * the longest across the voices struck; 0 when there are none.
   */
  int AnticipationTicks(const Hand &, const Tick &strike) const;
  /**
   * When the automatic player is to make the gesture striking the chords due
   * at `strike`, in ticks with repeats: ahead of them by their grace notes'
   * length (AnticipationTicks), and, if the hand is trilling, timed for the
   * trill step boundary nearest to that. A gesture takes effect when the
   * step under way ends, so one aimed at a boundary lands the resolution
   * within half a trill note of the beat, where one aimed at the beat itself
   * would fall anywhere in a step and wait out the rest of it. Never before
   * the events played last.
   */
  int AutoPlayStrikeTick(const Hand &, const Tick &strike);
  /**
   * Moves the track on to what the gesture struck — nothing for a release —
   * and returns the note events to send now. Returns none while the track is
   * mid-way through a timed ornament step: the move then waits for the step
   * to end, so that every note of a trill lasts its full length, the last one
   * included. A chord's closing notes, if any, sound first, the move
   * following them.
   */
  NoteEvents ProceedOnTrack(TrackIndex, const IChord *past,
                            std::optional<OrnamentSchedule> strike,
                            float velocity);
  /**
   * Starts sounding the schedule on the track from `start`: records it and
   * queues its first step's end if timed. Returns the first step's pitches,
   * for the caller to strike. The caller holds the mutex.
   */
  std::vector<int> Begin(TrackIndex, TrackSchedule &, OrnamentSchedule,
                         float velocity,
                         std::chrono::steady_clock::time_point start);
  /**
   * Ornament thread: the timed step on the track ended — sounds the next, or
   * makes the move a gesture asked for meanwhile.
   */
  void OnOrnamentStepDue(OrnamentStepDue);
  /** Queues the step's end in due-time order. The caller holds the mutex. */
  static void QueueOrnamentStep(std::deque<QueueEntry<OrnamentStepDue>> &,
                                OrnamentStepDue);
  /** Silences and forgets every track's schedule: a jump. */
  void CancelOrnaments();
  double NowMs() const;

  const InstrumentIndex m_instrument;

  Hand m_rightHand;
  Hand m_leftHand;
  //! The right hand's top (melody) voice, set only when the right hand has
  //! several voices. Lower voices are then attenuated so the melody stands out.
  const std::optional<TrackIndex> m_rightHandUpperVoice;
  const std::vector<const VoiceSequencer *> m_allVoices;
  const Tick m_finalTick;
  const PedalSequence m_pedalSequence;
  //! The span of m_pedalSequence the pedal is down in, if it is.
  std::optional<size_t> m_pedalSpan;

  // Read by the threads from the moment they start: initialized before them.
  bool m_finished = false;

  //! The pedal thread also carries the releases of notes struck under a pedal
  //! press still pending, held back until the press so that the pedal catches
  //! them.
  ThreadMembers<EventVariant> m_pedalThreadMembers;
  ThreadMembers<NoteEvent> m_noteThreadMembers;
  //! Its queue is kept sorted by due time. Its mutex also guards m_tracks
  //! and m_ornamentGeneration.
  ThreadMembers<OrnamentStepDue> m_ornamentThreadMembers;
  //! The tracks with a struck chord on record: every ornamented one, and a
  //! plain one struck while an ornament was under way.
  std::map<int /*track*/, TrackSchedule> m_tracks;
  unsigned m_ornamentGeneration = 0;
  std::thread m_pedalThread;
  std::thread m_noteThread;
  std::thread m_ornamentThread;
  const std::chrono::steady_clock::time_point m_epoch =
      std::chrono::steady_clock::now();

  bool m_pedalDown = false;
  //! When the pedal was last lifted.
  std::chrono::steady_clock::time_point m_pedalLiftTime{};
  //! Counts the input events, to tell the notes struck since the pedal was
  //! last lifted (`m_liftSerial`) — those a pending press is to catch.
  unsigned m_inputSerial = 0;
  unsigned m_liftSerial = 0;
  //! The input event in which each sounding note, by track and pitch, was
  //! struck.
  std::map<std::pair<int, int>, unsigned> m_strikeSerial;
  std::mt19937 m_rng{0};
  std::uniform_int_distribution<int> m_delayDist{0, 30000};   // microseconds
  std::uniform_int_distribution<int> m_velocityDist{90, 110}; // percents

  muse::ValCh<std::map<TrackIndex, ChordTransition>> m_transitions;
  muse::async::Channel<AutoPlayEvent> m_handNoteEvent;
  muse::async::Channel<EventVariant> m_outputEvent;
  muse::async::Channel<int /*tick*/, JumpReason> m_aboutToJumpPosition;
  int m_autoPlayTick = 0;

  bool m_velocityRecordingEnabled = false;
  //! Whether the score's ornaments are played by the app (the automatic
  //! mode); otherwise every chord is struck plain.
  bool m_ornamentsEnabled = true;
};
} // namespace dgk
