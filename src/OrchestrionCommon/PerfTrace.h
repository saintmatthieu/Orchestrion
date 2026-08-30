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

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

namespace dgk
{
/**
 * Timing probes for chasing latency at runtime — GUI-thread stalls, late
 * onsets, audio underruns — switched on by naming a file in the
 * ORCHESTRION_PERF_LOG environment variable. Unset, a probe is one branch on a
 * static flag. Set, each probe appends one line to that file:
 *
 *     <t_us> <thread-ish tag> <name> <value_us> [<extra>]
 *
 * with t the steady clock in microseconds, so lines written from any thread —
 * and from the MuseScore fork's audio driver, which has its own copy of this
 * probe writing the same format to the same file — line up on one time axis.
 * Point it at tmpfs (e.g. /dev/shm/orchestrion-perf.log): the file is drained
 * by a background thread, but a journalled disk can still stall that thread.
 * Analyse with buildscripts/perf_trace_report.py, ideally alongside
 * buildscripts/perf_thread_sampler.py's output.
 */
class PerfTrace
{
public:
  static bool enabled()
  {
    static const bool on = std::getenv("ORCHESTRION_PERF_LOG") != nullptr;
    return on;
  }

  static long long nowUs()
  {
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch())
        .count();
  }

  /**
   * Record \p name with a duration/lateness/etc. \p valueUs, and an optional
   * free-text \p extra (no newlines). Never touches the filesystem: the line
   * goes to a memory buffer a background thread drains — a probe that wrote
   * the file itself could block on the journal for 100+ ms and stall the very
   * thread it measures (seen: every probing thread queued on ext4).
   */
  static void event(const char *tag, const char *name, long long valueUs,
                    const char *extra = "")
  {
    if (!enabled())
      return;
    char line[256];
    const int n = std::snprintf(line, sizeof line, "%lld %s %s %lld %s\n",
                                nowUs(), tag, name, valueUs, extra);
    if (n > 0)
      Writer::append(
          line, static_cast<std::size_t>(std::min<int>(n, sizeof line - 1)));
  }

  /**
   * Times the enclosing scope and records it as \p name on destruction.
   */
  class Scope
  {
  public:
    Scope(const char *tag, const char *name) : _tag{tag}, _name{name}
    {
      if (enabled())
        _startUs = nowUs();
    }
    ~Scope()
    {
      if (enabled())
        event(_tag, _name, nowUs() - _startUs);
    }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

  private:
    const char *_tag;
    const char *_name;
    long long _startUs = 0;
  };

private:
  class Writer
  {
  public:
    static void append(const char *line, std::size_t n)
    {
      Writer *writer = instance();
      if (!writer)
        return;
      std::lock_guard lock{writer->_mutex};
      writer->_buffer.append(line, n);
    }

  private:
    Writer()
        : _fd{::open(std::getenv("ORCHESTRION_PERF_LOG"),
                     O_WRONLY | O_APPEND | O_CREAT, 0644)},
          _thread{[this] { run(); }}
    {
    }
    ~Writer()
    {
      {
        std::lock_guard lock{_mutex};
        _stop = true;
      }
      _cv.notify_one();
      _thread.join();
      if (_fd >= 0)
        ::close(_fd);
    }
    static Writer *instance()
    {
      // Nothing after static destruction: a late probe is dropped.
      static Writer *const writer = new Writer;
      static const struct Guard
      {
        ~Guard()
        {
          delete writer;
          destroyed() = true;
        }
      } guard;
      return destroyed() ? nullptr : writer;
    }
    static bool &destroyed()
    {
      static bool flag = false;
      return flag;
    }
    void run()
    {
      std::string out;
      while (true)
      {
        bool stop = false;
        {
          std::unique_lock lock{_mutex};
          _cv.wait_for(lock, std::chrono::milliseconds{100},
                       [this] { return _stop; });
          out.swap(_buffer);
          stop = _stop;
        }
        write(out);
        out.clear();
        if (stop)
          return;
      }
    }
    void write(const std::string &out) const
    {
      // Whole lines per chunk, so the fork's own writer interleaves cleanly.
      const char *p = out.data();
      std::size_t left = out.size();
      while (_fd >= 0 && left > 0)
      {
        const ssize_t n = ::write(_fd, p, left);
        if (n <= 0)
          return;
        p += n;
        left -= static_cast<std::size_t>(n);
      }
    }

    std::mutex _mutex;
    std::condition_variable _cv;
    std::string _buffer;
    bool _stop = false;
    int _fd = -1;
    std::thread _thread;
  };
};
} // namespace dgk
