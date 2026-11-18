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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

namespace dgk
{
/**
 * A temporary tool: when each note-on/off gesture passes each hand-off on its
 * way to the sound (see OrchestrionSynthesis/gestureToSound.md), and when the
 * audio callbacks run. On with --latency-trace <dir>, where notes.csv,
 * callbacks.csv and meta.txt are written on exit; analysed by
 * tools/latency_trace.py.
 *
 * Lock-free and allocation-free once made: each note's step is stamped once,
 * by the first thread to get there (a chord's notes, sent to every track's
 * synthesizer, pass several times); the callbacks are written by the audio
 * callback's thread only. Times are steady_clock's, in nanoseconds.
 */
class LatencyTrace
{
public:
  /** What played the note. */
  enum class Source
  {
    gesture,
    automatic, // AutomaticOrchestrionPlayer
  };

  enum Step
  {
    /**
     * GestureInput::Dispatch, on the input thread; the automatic player's,
     * when it was due.
     */
    dispatched,
    played,   // GestureInputConnector::onGesture, on the play thread
    queued,   // OrchestrionSequencer::QueueOutput, on the play thread
    sent,     // OrchestrionSequencer::SendOutput, on the output thread
    received, // OrchestrionSynthesizerWrapper::processEvent, audio thread
    numSteps
  };

  /**
   * Traces into this directory (made if it isn't there): before any thread
   * might trace, i.e. before the application is set up.
   */
  static void enable(const std::filesystem::path &dir)
  {
    LatencyTrace &trace = singleton();
    if (dir.empty() || !trace.m_dir.empty())
      return;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    trace.m_dir = dir;
    // Made, and their pages touched, before anything is traced.
    trace.m_notes = std::make_unique<Note[]>(noteCapacity);
    trace.m_callbacks = std::make_unique<Callback[]>(callbackCapacity);
  }

  /** Null unless tracing. */
  static LatencyTrace *instance()
  {
    LatencyTrace &trace = singleton();
    return trace.m_dir.empty() ? nullptr : &trace;
  }

  static std::int64_t now() { return toNs(std::chrono::steady_clock::now()); }

  static std::int64_t toNs(std::chrono::steady_clock::time_point t)
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               t.time_since_epoch())
        .count();
  }

  /**
   * A new note's id (never 0), or 0 when there's no room left. The automatic
   * player's hands have no pitch: -1 for the left, -2 for the right.
   */
  std::uint32_t newNote(bool noteOn, int pitch, std::int64_t dispatchedNs,
                        Source source = Source::gesture)
  {
    const std::uint32_t id = m_noteCount.fetch_add(1) + 1;
    if (id >= noteCapacity)
      return 0;
    Note &note = m_notes[id];
    note.noteOn.store(noteOn, std::memory_order_relaxed);
    note.pitch.store(pitch, std::memory_order_relaxed);
    note.automatic.store(source == Source::automatic,
                         std::memory_order_relaxed);
    stamp(id, dispatched, dispatchedNs);
    return id;
  }

  void stamp(std::uint32_t id, Step step, std::int64_t ns = now())
  {
    if (id == 0 || id >= noteCapacity)
      return;
    std::int64_t expected = 0;
    m_notes[id].t[step].compare_exchange_strong(expected, ns,
                                                std::memory_order_relaxed);
  }

  /** When the sequencer wants it sent, if later than now. */
  void setDue(std::uint32_t id, std::int64_t ns)
  {
    if (id == 0 || id >= noteCapacity)
      return;
    std::int64_t expected = 0;
    m_notes[id].due.compare_exchange_strong(expected, ns,
                                            std::memory_order_relaxed);
  }

  /** On the audio callback's thread. */
  void callback(std::int64_t startNs, std::int64_t endNs, int frames)
  {
    const std::uint64_t i = m_callbackCount.load(std::memory_order_relaxed);
    if (i >= callbackCapacity)
      return;
    m_callbacks[i] = {startNs, endNs, frames};
    m_callbackCount.store(i + 1, std::memory_order_release);
  }

  /** The output as opened, on the main thread. */
  void setOutput(const std::string &driver, int sampleRate, int frames)
  {
    m_driver = driver;
    m_sampleRate = sampleRate;
    m_requestedFrames = frames;
  }

  /**
   * The gesture the play thread is playing (see GestureInputConnector),
   * whose notes the sequencer queues meanwhile.
   */
  static std::uint32_t &current()
  {
    thread_local std::uint32_t id = 0;
    return id;
  }

  struct CurrentScope
  {
    explicit CurrentScope(std::uint32_t id) { current() = id; }
    ~CurrentScope() { current() = 0; }
  };

private:
  static constexpr std::uint32_t noteCapacity = 1u << 16;
  static constexpr std::uint64_t callbackCapacity = 1u << 20;

  struct Note
  {
    std::atomic<std::int64_t> t[numSteps]{};
    std::atomic<std::int64_t> due{0};
    std::atomic<int> pitch{0};
    std::atomic<bool> noteOn{false};
    std::atomic<bool> automatic{false};
  };

  struct Callback
  {
    std::int64_t start;
    std::int64_t end;
    int frames;
  };

  static FILE *open(const std::filesystem::path &path)
  {
#ifdef _WIN32
    return _wfopen(path.c_str(), L"w");
#else
    return std::fopen(path.c_str(), "w");
#endif
  }

  static LatencyTrace &singleton()
  {
    static LatencyTrace trace;
    return trace;
  }

  LatencyTrace() = default;

  ~LatencyTrace()
  {
    if (!m_dir.empty())
      dump();
  }

public:
  /**
   * Writes what was traced: on exit, or before, if the process ends without
   * destroying its statics (testflow's runner).
   */
  void dump() const
  {
    if (FILE *f = open(m_dir / "meta.txt"))
    {
      std::fprintf(f, "driver=%s\nsampleRate=%d\nrequestedFrames=%d\n",
                   m_driver.c_str(), m_sampleRate, m_requestedFrames);
      std::fclose(f);
    }
    if (FILE *f = open(m_dir / "notes.csv"))
    {
      std::fprintf(f, "id,source,kind,pitch,dispatched,played,queued,due,"
                      "sent,received\n");
      const std::uint32_t count =
          std::min<std::uint32_t>(m_noteCount.load() + 1, noteCapacity);
      for (std::uint32_t id = 1; id < count; ++id)
      {
        const Note &n = m_notes[id];
        std::fprintf(
            f, "%u,%s,%s,%d,%lld,%lld,%lld,%lld,%lld,%lld\n", id,
            n.automatic.load() ? "auto" : "gesture",
            n.noteOn.load() ? "on" : "off", n.pitch.load(),
            (long long)n.t[dispatched].load(), (long long)n.t[played].load(),
            (long long)n.t[queued].load(), (long long)n.due.load(),
            (long long)n.t[sent].load(), (long long)n.t[received].load());
      }
      std::fclose(f);
    }
    if (FILE *f = open(m_dir / "callbacks.csv"))
    {
      std::fprintf(f, "start,end,frames\n");
      const std::uint64_t count =
          m_callbackCount.load(std::memory_order_acquire);
      for (std::uint64_t i = 0; i < count; ++i)
        std::fprintf(f, "%lld,%lld,%d\n", (long long)m_callbacks[i].start,
                     (long long)m_callbacks[i].end, m_callbacks[i].frames);
      std::fclose(f);
    }
  }

private:
  std::filesystem::path m_dir;
  std::unique_ptr<Note[]> m_notes;
  std::atomic<std::uint32_t> m_noteCount{0};
  std::unique_ptr<Callback[]> m_callbacks;
  std::atomic<std::uint64_t> m_callbackCount{0};
  std::string m_driver;
  int m_sampleRate = 0;
  int m_requestedFrames = 0;
};
} // namespace dgk
