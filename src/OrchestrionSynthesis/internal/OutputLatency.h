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

#include "IOutputLatency.h"

#include <audio/iaudiooutputmonitor.h>
#include <audio/main/internal/iaudiodriverfactory.h>

#include <chrono>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>

class QTimer;

namespace dgk
{
/**
 * Polls the driver's monitor on the main thread, keeps the history the
 * dialog shows, and chooses the margin level (see IOutputLatency).
 *
 * The automatic setting raises the level after an underrun or a close call
 * (less than 1 ms of headroom), and lowers it
 * after five minutes of sound without one, if the least headroom over them
 * would still leave 2 ms with a period less. Its level is kept across
 * sessions, and so is the user's.
 */
class OutputLatency : public IOutputLatency
{
public:
  OutputLatency();
  ~OutputLatency() override;

  /**
   * The driver made last, from DriverCapturingFactory (any thread).
   */
  void setDriver(const muse::audio::IAudioDriverPtr &driver);

  /**
   * Main thread, once the application is up: starts polling.
   */
  void start();

  Status status() const override;
  muse::async::Notification statusChanged() const override;
  void setAutomatic(bool automatic) override;
  void setLevel(size_t level) override;
  void resetCounters() override;
  void delayNextCallback(double ms) override;

private:
  using Clock = std::chrono::steady_clock;

  struct Second
  {
    int underruns = 0;
    int clockStalls = 0;
  };

  struct Recent
  {
    Clock::time_point time;
    muse::audio::IAudioOutputMonitor::Window window;
  };

  std::shared_ptr<muse::audio::IAudioOutputMonitor> monitor() const;
  void poll();
  void applyWhenSilent(muse::audio::IAudioOutputMonitor &monitor,
                       const muse::audio::IAudioOutputMonitor::Window &window);
  void runAutomatic(const muse::audio::IAudioOutputMonitor::Window &window,
                    double periodMs, size_t levelCount);
  size_t wantedLevel(size_t levelCount) const;

  mutable std::mutex m_driverMutex;
  std::weak_ptr<muse::audio::IAudioOutputMonitor> m_monitor;
  std::string m_driverName;

  std::unique_ptr<QTimer> m_timer;
  muse::async::Notification m_statusChanged;

  bool m_automatic = true;
  size_t m_automaticLevel = 0;
  size_t m_userLevel = 0;

  // Main thread
  std::deque<Recent> m_recent; // the last 10 s
  std::deque<Second> m_seconds;
  Second m_thisSecond;
  Clock::time_point m_secondStart;
  uint64_t m_underruns = 0;
  uint64_t m_clockStalls = 0;
  std::optional<Clock::time_point> m_lastUnderrun;
  std::optional<Clock::time_point> m_silentSince;
  // The automatic setting's evidence for a lower level
  double m_soundingSecondsClean = 0.0;
  double m_cleanHeadroomMinMs = std::numeric_limits<double>::infinity();
  int m_raisesThisSession = 0;
};

/**
 * Hands each driver the given factory makes to OutputLatency.
 */
class DriverCapturingFactory : public muse::audio::IAudioDriverFactory
{
public:
  DriverCapturingFactory(
      std::shared_ptr<muse::audio::IAudioDriverFactory> factory,
      std::shared_ptr<OutputLatency> outputLatency)
      : m_factory{std::move(factory)}, m_outputLatency{std::move(outputLatency)}
  {
  }
  std::vector<std::string> availableAudioDrivers() const override
  {
    return m_factory->availableAudioDrivers();
  }
  muse::audio::IAudioDriverPtr
  createDriver(const std::string &name) const override
  {
    muse::audio::IAudioDriverPtr driver = m_factory->createDriver(name);
    if (driver)
      m_outputLatency->setDriver(driver);
    return driver;
  }

private:
  const std::shared_ptr<muse::audio::IAudioDriverFactory> m_factory;
  const std::shared_ptr<OutputLatency> m_outputLatency;
};
} // namespace dgk
