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

#include "OrchestrionTypes.h"
#include <async/channel.h>
#include <async/notification.h>
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
  std::optional<NoteEventType> leftHandEvent;
  std::optional<NoteEventType> rightHandEvent;
  /**
   * The notes of the staves in the Notes mode, played as keys are (see
   * IOrchestrionSequencer::OnInputEvent): followed, as the performer is.
   */
  std::vector<std::pair<NoteEventType, int /*pitch*/>> notes;
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
  Rewind,   // the rewind action, or a fresh sequencer starting at the top
  Step,     // the previous/next-note actions
  Click,    // the user clicked an element on the page
  LoopWrap, // the loop's end was reached: back to its start
  ScoreEnd, // the last event was played: back to the start
  Replay,   // the replay rewinds to the take's start
  Realign,  // the staves changed modes: back where the playing ones were
};

class IOrchestrionSequencer
{
public:
  virtual ~IOrchestrionSequencer() = default;

  /**
   * A key the performer played. It goes to the staff it is for, going by the
   * staff modes (see IStaffModes). A key of a Notes staff sounds as it is,
   * and the performer is followed in the score from it; one of a Rhythm
   * staff strikes its next chord. With a staff of each, the Rhythm one gets
   * the keys past the Notes one's range, a minor third clear of it; with two
   * Rhythm staves, they are split at middle C. If a note-on comes from a
   * controller that doesn't carry velocity, such as a computer keyboard, set
   * velocity to std::nullopt.
   */
  virtual void OnInputEvent(NoteEventType, int pitch,
                            std::optional<float> velocity) = 0;

  virtual void OnSustainPedal(bool down) = 0;
  /**
   * Whether the pedal follows the player's foot rather than the score's pedal
   * markings — which it does by default, for there is no telling whether a
   * pedal is plugged in until it is used.
   */
  virtual void SetFootPedalling(bool) = 0;

  /**
   * A gesture the machine makes on a hand's staff — the automatic player's,
   * the Follow mode's — whatever the staff's mode, but for a Mute one.
   */
  virtual void OnHandInputEvent(NoteEventType, bool isLeftHand,
                                std::optional<float> velocity) = 0;

  /**
   * Releases whatever is sounding — the hands' held keys and the sustain
   * pedal — as if the player had lifted hands and foot. Called when playback
   * is stopped, so the notes don't ring on. The performance's tempo estimate
   * goes with it (see PerformedTempoNow), so the Follow staves stop too.
   */
  virtual void AllNotesOff() = 0;
  virtual muse::async::Channel<std::map<TrackIndex, ChordTransition>>
  ChordTransitions() const = 0;
  //! Fires once per processed hand note event (both manual and automatic play),
  //! carrying the event type and which hand it belongs to. Used by the beginner
  //! help animation to press the matching key.
  virtual muse::async::Channel<AutoPlayEvent> HandNoteEvents() const = 0;
  virtual const std::map<TrackIndex, ChordTransition> &
  GetCurrentTransitions() const = 0;
  virtual std::vector<TrackIndex> GetAllVoices() const = 0;
  /**
   * The chord the track's next note-on strikes, null if none is left: what
   * a transition leaves out when the track moves onto a rest.
   */
  virtual const IChord *GetNextChord(TrackIndex) const = 0;
  virtual muse::async::Channel<EventVariant> OutputEvent() const = 0;
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
  virtual void GoToTick(int tick, JumpReason reason) = 0;
  /**
   * Moves the hand on, silently, to the first of its chords at or after the
   * tick (with repeats), letting go of what it holds: what a Follow staff
   * leaves out while there is no tempo to follow.
   */
  virtual void SkipHandTo(bool isLeftHand, int tickWithRepeats) = 0;

  /**
   * The performance now, estimated from the onsets of the staves the
   * performer plays (not the Follow ones): one estimate for them all, which
   * the Follow staves play to and the ornaments are timed with. Empty until
   * two onsets give a tempo — at first, after a jump, after a pause.
   */
  virtual std::optional<PerformedTempo> PerformedTempoNow() = 0;
  virtual void GoToPrevNoteonTick() = 0;
  virtual void GoToNextNoteonTick() = 0;

  /**
   * Returns the next batch of input events to play, with a delta tick count
   * relative to the previous call (0 at the start of the score). A chord with
   * grace notes before it is due early by their length, as a performer plays
   * it, so that the chord itself falls on its beat. Only the staves the
   * performer would play: not the Follow ones, which follow the player, nor
   * the Mute ones. Returns std::nullopt when there is nothing left to play.
   */
  virtual std::optional<NextAutoPlayEvents> WhatToPlayNext() = 0;
};

using IOrchestrionSequencerPtr = std::shared_ptr<IOrchestrionSequencer>;
} // namespace dgk