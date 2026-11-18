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

#include "EventRing.h"
#include "OrchestrionTypes.h"

#include <modularity/imoduleinterface.h>

#include <memory>

namespace dgk
{
/**
 * What is to sound, from the sequencer to the synthesizers: sent on the
 * sequencer's output thread, read by each synthesizer on the audio callback's
 * thread, as it renders its next block. No thread in between: the audio
 * engine's worker, which a kors channel would go through, only looks at its
 * messages every 16 ms (see OrchestrionSynthesis/gestureToSound.md).
 *
 * The sequencers come and go with the scores; this stays, and so do the
 * synthesizers' connections to it.
 */
class ISynthesizerFeed : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(ISynthesizerFeed);

public:
  virtual ~ISynthesizerFeed() = default;

  /** On the sequencer's output thread, its only sender. */
  virtual void send(const EventVariant &) = 0;

  /**
   * A ring that every event sent from now on is pushed to, until the
   * caller lets go of it.
   */
  virtual std::shared_ptr<EventRing> connect() = 0;
};
} // namespace dgk
