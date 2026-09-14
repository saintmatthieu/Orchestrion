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

#include <atomic>
#include <mutex>

namespace dgk
{
/**
 * A value set by the main thread (from the parameters file) and read by the
 * audio thread without ever waiting.
 */
template <typename T> class ParameterStore
{
public:
  explicit ParameterStore(T initial) : m_value{std::move(initial)} {}

  void set(const T &value)
  {
    {
      const std::lock_guard lock(m_mutex);
      m_value = value;
    }
    m_version.fetch_add(1, std::memory_order_release);
  }

  /**
   * Audio thread: copies the value if it changed since `seenVersion`
   * (updated), returning whether it did. Skips (and reports no change) when
   * the main thread holds the lock; the next block catches up.
   */
  bool readIfChanged(T &out, unsigned &seenVersion) const
  {
    const unsigned version = m_version.load(std::memory_order_acquire);
    if (version == seenVersion)
      return false;
    const std::unique_lock lock(m_mutex, std::try_to_lock);
    if (!lock.owns_lock())
      return false;
    out = m_value;
    seenVersion = version;
    return true;
  }

private:
  mutable std::mutex m_mutex;
  std::atomic<unsigned> m_version{1};
  T m_value;
};
} // namespace dgk
