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
  if (opened)
    if (LatencyTrace *const trace = LatencyTrace::instance())
    {
      const Spec &active = m_driver->activeSpec();
      trace->setOutput(m_driver->name() + " " + active.deviceId,
                       static_cast<int>(active.output.sampleRate),
                       static_cast<int>(active.output.samplesPerChannel));
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
