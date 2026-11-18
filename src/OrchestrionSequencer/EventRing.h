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

#include "OrchestrionTypes.h"

#include <atomic>
#include <cstddef>
#include <vector>

namespace dgk
{
/**
 * A single-producer single-consumer queue of the sequencer's output, from its
 * output thread to the audio callback's (see ISynthesizerFeed), lock-free.
 *
 * The consumer reads the events where they are and leaves them there: a slot
 * is overwritten, and what it held freed, by the producer, when it comes
 * round to it again. Nothing is allocated nor freed on the audio thread.
 */
class EventRing
{
public:
  explicit EventRing(std::size_t capacity) : m_slots(capacity + 1) {}

  EventRing(const EventRing &) = delete;
  EventRing &operator=(const EventRing &) = delete;

  /**
   * On the producer's thread. When full (the consumer isn't reading), the
   * event is dropped, and the consumer told (see drain).
   */
  void push(const EventVariant &event)
  {
    const std::size_t write = m_write.load(std::memory_order_relaxed);
    const std::size_t next = (write + 1) % m_slots.size();
    if (next == m_read.load(std::memory_order_acquire))
    {
      m_overflowed.store(true, std::memory_order_release);
      return;
    }
    m_slots[write] = event;
    m_write.store(next, std::memory_order_release);
  }

  /**
   * On the consumer's thread: calls f(const EventVariant &) for each event
   * pushed so far, in order. If the ring was full since the last call,
   * events were dropped, and those left are stale (the consumer didn't read
   * for a long while): they are skipped, and false returned, for the
   * consumer to silence what may have been left sounding.
   */
  template <typename F> bool drain(F &&f)
  {
    std::size_t read = m_read.load(std::memory_order_relaxed);
    const std::size_t write = m_write.load(std::memory_order_acquire);
    if (m_overflowed.exchange(false, std::memory_order_acq_rel))
    {
      m_read.store(write, std::memory_order_release);
      return false;
    }
    while (read != write)
    {
      f(static_cast<const EventVariant &>(m_slots[read]));
      read = (read + 1) % m_slots.size();
      m_read.store(read, std::memory_order_release);
    }
    return true;
  }

private:
  std::vector<EventVariant> m_slots;
  std::atomic<std::size_t> m_write{0};
  std::atomic<std::size_t> m_read{0};
  std::atomic<bool> m_overflowed{false};
};
} // namespace dgk
