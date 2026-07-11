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

class IOrchestrionSequencer
{
public:
  virtual ~IOrchestrionSequencer() = default;

  //! If a note-on comes from a controller that doesn't carry velocity, such as
  //! a computer keyboard, set velocity to std::nullopt.
  virtual void OnInputEvent(NoteEventType, int pitch,
                            std::optional<float> velocity) = 0;
  virtual muse::async::Channel<std::map<TrackIndex, ChordTransition>>
  ChordTransitions() const = 0;
  //! Fires once per processed hand note event (both manual and automatic play),
  //! carrying the event type and which hand it belongs to. Used by the beginner
  //! help animation to press the matching key.
  virtual muse::async::Channel<AutoPlayEvent> HandNoteEvents() const = 0;
  virtual const std::map<TrackIndex, ChordTransition> &
  GetCurrentTransitions() const = 0;
  virtual std::vector<TrackIndex> GetAllVoices() const = 0;
  virtual muse::async::Channel<EventVariant> OutputEvent() const = 0;
  virtual muse::async::Channel<int /*tick*/> AboutToJumpPosition() const = 0;
  virtual void GoToTick(int tick) = 0;
  virtual void GoToPrevNoteonTick() = 0;
  virtual void GoToNextNoteonTick() = 0;

  //! Returns the next batch of input events to play, with a delta tick count
  //! relative to the previous call (0 at the start of the score). Returns
  //! std::nullopt when there is nothing left to play.
  virtual std::optional<NextAutoPlayEvents> WhatToPlayNext() = 0;
};

using IOrchestrionSequencerPtr = std::shared_ptr<IOrchestrionSequencer>;
} // namespace dgk