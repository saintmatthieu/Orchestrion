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

#include "GestureControllers/IGestureInput.h"
#include "OrchestrionTypes.h"
#include "Performance.h"
#include "RecordingTimeline.h"
#include <async/channel.h>
#include <async/notification.h>
#include <chrono>
#include <map>
#include <optional>

namespace dgk
{

struct AutoPlayEvent
{
  NoteEventType type;
  bool isLeftHand;
  //! The gesture's raw controller velocity — empty when the input device
  //! doesn't measure one (e.g. the computer keyboard) and for note-offs.
  //! Consumers judging played dynamics must ignore derived velocities, so
  //! only the genuine measurement travels here.
  std::optional<float> velocity;
};

struct NextAutoPlayEvents
{
  int deltaTicks = 0;
  /** When they are due, in ticks with repeats from the start of the score. */
  int tick = 0;
  std::optional<NoteEventType> leftHandEvent;
  std::optional<NoteEventType> rightHandEvent;
  /**
   * The notes of the staves in the Notes mode, played as keys are (see
   * OrchestrionSequencer::OnInputEvent): followed, as the performer is.
   */
  std::vector<std::pair<NoteEventType, int /*pitch*/>> notes;
  /** The sustain pedal, as a performance played has it. */
  std::optional<bool> pedal;
};

//! One raw input event of a recorded take, for replaying the performance:
//! when it happened (relative to the take's first event) and the same
//! routing and velocity information the live gesture carried.
struct ReplayEvent
{
  int ms = 0;
  NoteEventType type = NoteEventType::noteOn;
  bool isLeftHand = false;
  std::optional<float> velocity;
};

//! A finished take, replayable by the automatic player: the earliest score
//! tick it struck — where the replay rewinds to — and its input events.
struct ReplayTake
{
  int startTick = 0;
  std::vector<ReplayEvent> events;
};

/** Where the performance is, and how fast it goes. */
struct PerformedTempo
{
  /** Ticks with repeats. */
  double tick = 0.0;
  double ticksPerMs = 0.0;
  /** Whether the next onset is overdue, the estimate coasting to a stop. */
  bool coasting = false;
};

enum class JumpReason
{
  Rewind,    // the rewind action, or a fresh sequencer starting at the top
  Step,      // the previous/next-note actions
  Click,     // the user clicked an element on the page
  LoopWrap,  // the loop's end was reached: back to its start
  ScoreEnd,  // the last event was played: back to the start
  End,       // the go-to-end action: past the last event
  Replay,    // the replay rewinds to the take's start
  Realign,   // the staves changed modes: back where the playing ones were
  Recording, // following a recording: where it is, or goes on after a skip
};

/**
 * The sequencer as the main thread and the audio engine see it (see
 * internal/SequencerProxy.h): where it is and what it tells, and going
 * elsewhere in the score. Playing it (gestures, the pedal) is for what
 * lives on the play thread with it: see internal/OrchestrionSequencer.h.
 */
class IOrchestrionSequencer
{
public:
  virtual ~IOrchestrionSequencer() = default;

  virtual muse::async::Channel<std::map<TrackIndex, ChordTransition>>
  ChordTransitions() const = 0;
  //! Fires once per processed hand note event (both manual and automatic play),
  //! carrying the event type and which hand it belongs to. Used by the beginner
  //! help animation to press the matching key.
  virtual muse::async::Channel<AutoPlayEvent> HandNoteEvents() const = 0;
  virtual std::map<TrackIndex, ChordTransition>
  GetCurrentTransitions() const = 0;
  virtual std::vector<TrackIndex> GetAllVoices() const = 0;
  /**
   * The chord the track's next note-on strikes, null if none is left: what
   * a transition leaves out when the track moves onto a rest.
   */
  virtual const IChord *GetNextChord(TrackIndex) const = 0;
  /**
   * What is to sound, sent on the main thread, for what shows it or sends it
   * out from there (the pedal indicator, MIDI out): the sequencer's threads
   * send nothing to the main thread through kors, which may make the sender
   * wait for it. The synthesizers get it through ISynthesizerFeed.
   */
  virtual muse::async::Channel<EventVariant> MainThreadOutputEvent() const = 0;
  /**
   * The chords the performer is playing a wrong note against: those of the
   * keys held that aren't one of the notes where the performer is. Sent
   * whenever that changes; empty when no wrong key is held.
   */
  virtual muse::async::Channel<std::vector<const IChord *>>
  WrongNotes() const = 0;
  /**
   * Sent before the position jumps to the given tick, and before the
   * transitions batch the jump causes, with the reason for it.
   */
  virtual muse::async::Channel<int /*tick*/, JumpReason>
  AboutToJumpPosition() const = 0;
  /**
   * The performer's gestures (see IGestureInput), each told before what it
   * causes (transitions, ...): so that a take records where it began, and
   * when each gesture was made rather than when the main thread heard of it.
   */
  virtual muse::async::Channel<IGestureInput::Gesture> Gestures() const = 0;
  /**
   * When what is being told on the channels above happened, on the steady
   * clock. What reaches the main thread comes as late as it is busy: what
   * goes by time (judging the timing of onsets) goes by this rather than by
   * when it is told. Meant to be asked while being told.
   */
  virtual std::chrono::steady_clock::time_point EventTime() const = 0;
  virtual void GoToTick(int tick, JumpReason reason) = 0;
  /**
   * Goes past the last event (JumpReason::End), where playing the score
   * through leaves it: nothing is to come, and the next note starts it over.
   */
  virtual void GoToEnd() = 0;
  /**
   * The performance now, estimated from the onsets of the staves the
   * performer plays (not the Follow ones): one estimate for them all, which
   * the Follow staves play to and the ornaments are timed with. Empty until
   * two onsets give a tempo — at first, after a jump, after a pause.
   */
  virtual std::optional<PerformedTempo> PerformedTempoNow() = 0;
  virtual void GoToPrevNoteonTick() = 0;
  virtual void GoToNextNoteonTick() = 0;

  /** What a key of a MIDI keyboard plays (see
   * OrchestrionSequencer::OnInputEvent). */
  enum class KeyRole
  {
    /** Nothing */
    none,
    /** The left hand's next chord */
    leftHand,
    /** The right hand's next chord */
    rightHand,
    /** Its own pitch, of a staff in the Notes mode */
    notes,
  };
  virtual KeyRole RoleOfKey(int pitch) const = 0;

  /**
   * Plays the score as `performance` (a recording, transcribed) was: each
   * note as loud as it was, the loudest of a hand's strike as loud as the
   * gesture (the automatic player's, as performed); and, the automatic
   * player playing, its pedal rather than the score's. None (no notes): as
   * the score says. Returns when the performance played each part of the
   * score, for the automatic player (IOrchestrionPlayer::SetTempoCurve);
   * null if it plays none of it.
   */
  virtual std::shared_ptr<const RecordingTimeline>
  PlayPerformance(Performance performance) = 0;
};

using IOrchestrionSequencerPtr = std::shared_ptr<IOrchestrionSequencer>;
} // namespace dgk