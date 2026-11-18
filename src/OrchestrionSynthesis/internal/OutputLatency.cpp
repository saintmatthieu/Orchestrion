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
#include "OutputLatency.h"

#include <global/settings.h>
#include <log.h>

#include <QTimer>
#include <algorithm>
#include <cmath>
#include <limits>

namespace dgk
{
namespace
{
const std::string moduleName("OrchestrionSynthesis");
const muse::Settings::Key AUTOMATIC(moduleName, "latency/automatic");
const muse::Settings::Key AUTOMATIC_LEVEL(moduleName, "latency/automaticLevel");
const muse::Settings::Key USER_LEVEL(moduleName, "latency/userLevel");

constexpr auto pollInterval = std::chrono::milliseconds{250};
constexpr auto recentSpan = std::chrono::seconds{10};
constexpr size_t historySeconds = 120;
// Below this the output is taken as silent (-80 dBFS)
constexpr float silentPeak = 1e-4f;
constexpr auto silentFor = std::chrono::milliseconds{500};
// The automatic setting lowers its level after so much sound without an
// underrun (doubled after each raise in the session), if the least headroom
// over it would still leave this much with a period less
constexpr double cleanSecondsToLower = 300.0;
constexpr double headroomToLowerMs = 2.0;
// Less headroom than this is as good as an underrun: the automatic setting
// raises its level
constexpr double closeCallMs = 1.0;
} // namespace

OutputLatency::OutputLatency() = default;

OutputLatency::~OutputLatency() = default;

void OutputLatency::setDriver(const muse::audio::IAudioDriverPtr &driver)
{
  std::scoped_lock lock{m_driverMutex};
  m_monitor =
      std::dynamic_pointer_cast<muse::audio::IAudioOutputMonitor>(driver);
  m_driverName = driver->name();
}

std::shared_ptr<muse::audio::IAudioOutputMonitor> OutputLatency::monitor() const
{
  std::scoped_lock lock{m_driverMutex};
  return m_monitor.lock();
}

void OutputLatency::start()
{
  muse::settings()->setDefaultValue(AUTOMATIC, muse::Val(true));
  muse::settings()->setDefaultValue(AUTOMATIC_LEVEL, muse::Val(0));
  muse::settings()->setDefaultValue(USER_LEVEL, muse::Val(0));
  m_automatic = muse::settings()->value(AUTOMATIC).toBool();
  m_automaticLevel =
      std::max(0, muse::settings()->value(AUTOMATIC_LEVEL).toInt());
  m_userLevel = std::max(0, muse::settings()->value(USER_LEVEL).toInt());

  m_secondStart = Clock::now();
  m_timer = std::make_unique<QTimer>();
  m_timer->setInterval(pollInterval);
  QObject::connect(m_timer.get(), &QTimer::timeout, [this] { poll(); });
  m_timer->start();
}

size_t OutputLatency::wantedLevel(size_t levelCount) const
{
  if (levelCount == 0)
    return 0;
  return std::min(m_automatic ? m_automaticLevel : m_userLevel, levelCount - 1);
}

void OutputLatency::poll()
{
  const std::shared_ptr<muse::audio::IAudioOutputMonitor> mon = monitor();
  if (!mon)
    return;

  const auto now = Clock::now();
  const muse::audio::IAudioOutputMonitor::Window window = mon->takeWindow();
  const double periodMs = mon->periodMs();
  const size_t levelCount = mon->marginLevelsMs().size();

  m_recent.push_back({now, window});
  while (!m_recent.empty() && now - m_recent.front().time > recentSpan)
    m_recent.pop_front();

  m_underruns += window.underruns;
  m_clockStalls += window.clockStalls;
  m_thisSecond.underruns += static_cast<int>(window.underruns);
  m_thisSecond.clockStalls += static_cast<int>(window.clockStalls);
  if (window.underruns > 0)
    m_lastUnderrun = now;
  for (int i = 0; now - m_secondStart >= std::chrono::seconds{1}; ++i)
  {
    m_seconds.push_back(i == 0 ? m_thisSecond : Second{});
    m_thisSecond = {};
    m_secondStart += std::chrono::seconds{1};
    while (m_seconds.size() > historySeconds)
      m_seconds.pop_front();
    if (i > static_cast<int>(historySeconds))
      m_secondStart = now;
  }

  runAutomatic(window, periodMs, levelCount);
  applyWhenSilent(*mon, window);
  m_statusChanged.notify();
}

void OutputLatency::runAutomatic(
    const muse::audio::IAudioOutputMonitor::Window &window, double periodMs,
    size_t levelCount)
{
  const bool closeCall =
      window.callbacks > 0 && window.headroomMinMs < closeCallMs;
  if (window.underruns > 0 || closeCall)
  {
    m_soundingSecondsClean = 0.0;
    m_cleanHeadroomMinMs = std::numeric_limits<double>::infinity();
    if (m_automatic && m_automaticLevel + 1 < levelCount)
    {
      ++m_automaticLevel;
      ++m_raisesThisSession;
      muse::settings()->setSharedValue(
          AUTOMATIC_LEVEL, muse::Val(static_cast<int>(m_automaticLevel)));
      LOGI() << (window.underruns > 0 ? "Underrun" : "Close call")
             << " (least headroom " << window.headroomMinMs
             << " ms): the output's margin goes up to level "
             << m_automaticLevel << " when it is next silent";
    }
    return;
  }

  const bool sounding = window.callbacks > 0 && window.outputPeak >= silentPeak;
  if (!sounding || levelCount == 0)
    return;
  const std::shared_ptr<muse::audio::IAudioOutputMonitor> mon = monitor();
  if (!mon || mon->marginLevel() != wantedLevel(levelCount))
    return;

  m_soundingSecondsClean += std::chrono::duration<double>(pollInterval).count();
  m_cleanHeadroomMinMs = std::min(m_cleanHeadroomMinMs, window.headroomMinMs);

  const double neededSeconds =
      cleanSecondsToLower * std::pow(2.0, m_raisesThisSession);
  if (m_automatic && m_automaticLevel > 0 &&
      m_soundingSecondsClean >= neededSeconds &&
      m_cleanHeadroomMinMs - periodMs >= headroomToLowerMs)
  {
    --m_automaticLevel;
    muse::settings()->setSharedValue(
        AUTOMATIC_LEVEL, muse::Val(static_cast<int>(m_automaticLevel)));
    LOGI() << "No underrun in " << m_soundingSecondsClean
           << " s of sound, least headroom " << m_cleanHeadroomMinMs
           << " ms: the output's margin goes down to level " << m_automaticLevel
           << " when it is next silent";
    m_soundingSecondsClean = 0.0;
    m_cleanHeadroomMinMs = std::numeric_limits<double>::infinity();
  }
}

void OutputLatency::applyWhenSilent(
    muse::audio::IAudioOutputMonitor &monitor,
    const muse::audio::IAudioOutputMonitor::Window &window)
{
  const auto now = Clock::now();
  // Not running: as good as silent
  const bool silent = window.callbacks == 0 || window.outputPeak < silentPeak;
  if (!silent)
  {
    m_silentSince.reset();
    return;
  }
  if (!m_silentSince)
    m_silentSince = now;

  const size_t levelCount = monitor.marginLevelsMs().size();
  if (levelCount == 0 || now - *m_silentSince < silentFor)
    return;
  const size_t wanted = wantedLevel(levelCount);
  if (monitor.marginLevel() == wanted)
    return;
  LOGI() << "The output is silent: its margin is now level " << wanted;
  monitor.setMarginLevel(wanted);
  m_soundingSecondsClean = 0.0;
  m_cleanHeadroomMinMs = std::numeric_limits<double>::infinity();
}

IOutputLatency::Status OutputLatency::status() const
{
  Status s;
  const std::shared_ptr<muse::audio::IAudioOutputMonitor> mon = monitor();
  if (!mon)
    return s;
  {
    std::scoped_lock lock{m_driverMutex};
    s.driver = m_driverName;
  }
  s.levelsMarginMs = mon->marginLevelsMs();
  s.available = !s.levelsMarginMs.empty();
  s.periodMs = mon->periodMs();
  s.automatic = m_automatic;
  s.level = wantedLevel(s.levelsMarginMs.size());
  s.appliedLevel = mon->marginLevel();
  uint64_t callbacks = 0;
  double headroomSum = 0.0;
  double loadSum = 0.0;
  s.headroomMinMs = std::numeric_limits<double>::infinity();
  for (const Recent &r : m_recent)
  {
    if (r.window.callbacks == 0)
      continue;
    callbacks += r.window.callbacks;
    headroomSum += r.window.headroomMeanMs * r.window.callbacks;
    loadSum += r.window.loadMeanPercent * r.window.callbacks;
    s.headroomMinMs = std::min(s.headroomMinMs, r.window.headroomMinMs);
    s.loadMaxPercent = std::max(s.loadMaxPercent, r.window.loadMaxPercent);
  }
  if (callbacks > 0)
  {
    s.headroomMeanMs = headroomSum / callbacks;
    s.loadMeanPercent = loadSum / callbacks;
  }
  else
    s.headroomMinMs = 0.0;

  s.underruns = m_underruns;
  s.clockStalls = m_clockStalls;
  if (m_lastUnderrun)
    s.secondsSinceLastUnderrun =
        std::chrono::duration<double>(Clock::now() - *m_lastUnderrun).count();
  s.underrunsPerSecond.assign(historySeconds - m_seconds.size(), 0);
  s.clockStallsPerSecond.assign(historySeconds - m_seconds.size(), 0);
  for (const Second &second : m_seconds)
  {
    s.underrunsPerSecond.push_back(second.underruns);
    s.clockStallsPerSecond.push_back(second.clockStalls);
  }
  return s;
}

muse::async::Notification OutputLatency::statusChanged() const
{
  return m_statusChanged;
}

void OutputLatency::setAutomatic(bool automatic)
{
  if (m_automatic == automatic)
    return;
  m_automatic = automatic;
  muse::settings()->setSharedValue(AUTOMATIC, muse::Val(automatic));
  m_statusChanged.notify();
}

void OutputLatency::setLevel(size_t level)
{
  m_userLevel = level;
  muse::settings()->setSharedValue(USER_LEVEL,
                                   muse::Val(static_cast<int>(level)));
  setAutomatic(false);
  m_statusChanged.notify();
}

void OutputLatency::resetCounters()
{
  m_underruns = 0;
  m_clockStalls = 0;
  m_lastUnderrun.reset();
  m_seconds.clear();
  m_thisSecond = {};
  // The automatic setting starts again from the least margin
  m_automaticLevel = 0;
  m_raisesThisSession = 0;
  m_soundingSecondsClean = 0.0;
  m_cleanHeadroomMinMs = std::numeric_limits<double>::infinity();
  muse::settings()->setSharedValue(AUTOMATIC_LEVEL, muse::Val(0));
  m_statusChanged.notify();
}

void OutputLatency::delayNextCallback(double ms)
{
  if (const std::shared_ptr<muse::audio::IAudioOutputMonitor> mon = monitor())
    mon->delayNextCallback(ms);
}
} // namespace dgk
