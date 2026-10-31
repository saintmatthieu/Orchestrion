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

#include <audio/iaudiodriver.h>
#include <audio/main/internal/iaudiodriverfactory.h>

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

namespace dgk
{
/**
 * The audio output while a video is rendered offline (see OfflineRendering):
 * no device, the engine's blocks are asked for on the main thread's clock,
 * which is the virtual one then, and written to a file. The engine runs on a
 * thread of its own, but only while the main thread waits for its block, so
 * that what it renders doesn't depend on how fast either thread runs.
 */
class OfflineAudioDriver : public muse::audio::IAudioDriver
{
public:
  ~OfflineAudioDriver() override;

  void init() override {}
  std::string name() const override { return "Offline"; }
  muse::audio::AudioDeviceID defaultDevice() const override
  {
    return "offline";
  }
  bool open(const Spec &spec, Spec *activeSpec) override;
  void close() override;
  bool isOpened() const override { return m_opened; }
  const Spec &activeSpec() const override { return m_spec; }
  muse::async::Channel<Spec> activeSpecChanged() const override
  {
    return m_activeSpecChanged;
  }
  std::vector<muse::audio::samples_t>
  availableOutputDeviceBufferSizes() const override;
  std::vector<muse::audio::sample_rate_t>
  availableOutputDeviceSampleRates() const override;
  muse::audio::AudioDeviceList availableOutputDevices() const override;
  muse::async::Notification availableOutputDevicesChanged() const override
  {
    return m_devicesChanged;
  }

private:
  void scheduleNextBlock();
  void renderBlock();
  void engineThread();

  Spec m_spec;
  bool m_opened = false;
  muse::async::Channel<Spec> m_activeSpecChanged;
  muse::async::Notification m_devicesChanged;

  std::chrono::steady_clock::time_point m_start;
  long long m_blocks = 0;
  std::vector<float> m_buffer;
  FILE *m_file = nullptr;

  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  bool m_requested = false;
  bool m_done = false;
  bool m_quit = false;
};

/**
 * Creates OfflineAudioDriver, whatever driver is asked for.
 */
class OfflineAudioDriverFactory : public muse::audio::IAudioDriverFactory
{
public:
  std::vector<std::string> availableAudioDrivers() const override
  {
    return {"Offline"};
  }
  muse::audio::IAudioDriverPtr createDriver(const std::string &) const override;
};
} // namespace dgk
