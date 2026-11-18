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
#include "TracingAudioDriver.h"
#include "OrchestrionCommon/LatencyTrace.h"

#include <algorithm>
#include <string>
#include <utility>

#include <QSysInfo>

#include <log.h>

namespace dgk
{
namespace
{
/**
 * The engine's callback, timed. Told apart from others when given back to
 * open() (the controller reopens with the active spec's), so as not to be
 * wrapped twice.
 */
struct TimedCallback
{
  muse::audio::IAudioDriver::Callback callback;
  int bytesPerFrame;

  void operator()(uint8_t *stream, int len) const
  {
    const auto start = LatencyTrace::now();
    callback(stream, len);
    if (LatencyTrace *const trace = LatencyTrace::instance())
      trace->callback(start, LatencyTrace::now(), len / bytesPerFrame);
  }
};
} // namespace

TracingAudioDriver::TracingAudioDriver(muse::audio::IAudioDriverPtr driver)
    : m_driver{std::move(driver)}
{
}

bool TracingAudioDriver::open(const Spec &spec, Spec *activeSpec)
{
  Spec timed = spec;
  const auto *alreadyTimed = spec.callback.target<TimedCallback>();
  const int channels =
      std::max(1, static_cast<int>(spec.output.audioChannelCount));
  timed.callback =
      TimedCallback{alreadyTimed ? alreadyTimed->callback : spec.callback,
                    channels * static_cast<int>(sizeof(float))};
  const bool opened = m_driver->open(timed, activeSpec);
  if (LatencyTrace *const trace = LatencyTrace::instance())
  {
    const auto list = [](const auto &values)
    {
      std::string s;
      for (const auto &v : values)
        s += (s.empty() ? "" : " ") + std::to_string(v);
      return s;
    };
    const Spec &active = m_driver->activeSpec();
    const std::pair<std::string, std::string> meta[] = {
        {"os", QSysInfo::prettyProductName().toStdString() + " (" +
                   QSysInfo::kernelType().toStdString() + " " +
                   QSysInfo::kernelVersion().toStdString() + ", " +
                   QSysInfo::currentCpuArchitecture().toStdString() + ")"},
        {"driver", m_driver->name()},
        {"opened", opened ? "yes" : "no"},
        {"opens", std::to_string(++m_opens)},
        {"askedDevice", spec.deviceId},
        {"askedSampleRate", std::to_string(spec.output.sampleRate)},
        {"askedFrames", std::to_string(spec.output.samplesPerChannel)},
        {"device", active.deviceId},
        {"sampleRate", std::to_string(active.output.sampleRate)},
        {"requestedFrames", std::to_string(active.output.samplesPerChannel)},
        {"channels", std::to_string(active.output.audioChannelCount)},
        {"availableSampleRates",
         list(m_driver->availableOutputDeviceSampleRates())},
        {"availableBufferSizes",
         list(m_driver->availableOutputDeviceBufferSizes())},
    };
    std::string line;
    for (const auto &[key, value] : meta)
    {
      trace->setMeta(key, value);
      line += " " + key + "=" + value;
    }
    LOGI() << "Latency trace, output opened:" << line;
  }
  return opened;
}

muse::audio::IAudioDriverPtr
TracingAudioDriverFactory::createDriver(const std::string &name) const
{
  muse::audio::IAudioDriverPtr driver = m_factory->createDriver(name);
  if (!driver)
    return driver;
  return std::make_shared<TracingAudioDriver>(std::move(driver));
}
} // namespace dgk
