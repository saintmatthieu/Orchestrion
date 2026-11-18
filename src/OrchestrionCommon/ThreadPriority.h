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

#if defined(__APPLE__)
#include <pthread/qos.h>
#endif

namespace dgk
{
/**
 * Called by a thread whose work is timed (it waits until something is due,
 * or for input) as it starts: it runs above normal priority where the
 * platform allows (see src/OrchestrionSequencer/threading.md, "What's left").
 * Never on the main thread.
 *
 * - macOS: user-interactive QoS. A default-QoS thread's timed waits may be
 *   coalesced: they wake up to 10 ms late.
 * - Linux, Windows: not yet (normal priority).
 */
inline void raiseTimedThreadPriority()
{
#if defined(__APPLE__)
  pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
}
} // namespace dgk
