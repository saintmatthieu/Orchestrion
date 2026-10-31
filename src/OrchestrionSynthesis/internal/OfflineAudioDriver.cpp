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
#include "OfflineAudioDriver.h"
#include "OrchestrionCommon/OfflineRendering.h"

#include <async/processevents.h>

#include <QCoreApplication>
#include <QTimer>

namespace dgk
{
using namespace std::chrono;

OfflineAudioDriver::~OfflineAudioDriver() { close(); }

bool OfflineAudioDriver::open(const Spec &spec, Spec *activeSpec)
{
  if (m_opened)
    close();
  m_spec = spec;
  m_spec.deviceId = defaultDevice();
  if (activeSpec)
    *activeSpec = m_spec;
  m_buffer.resize(m_spec.output.samplesPerChannel *
                  m_spec.output.audioChannelCount);
  m_file = std::fopen(offlineRendering.audioPath.c_str(), "wb");
  m_quit = false;
  m_thread = std::thread{[this] { engineThread(); }};
  m_opened = true;
  // On the main thread's clock (the virtual one)
  m_start = steady_clock::now();
  m_blocks = 0;
  offlineRendering.audioStart = m_start;
  offlineRendering.sampleRate = int(m_spec.output.sampleRate);
  offlineRendering.channelCount = int(m_spec.output.audioChannelCount);
  scheduleNextBlock();
  return true;
}

void OfflineAudioDriver::close()
{
  if (!m_opened)
    return;
  m_opened = false;
  {
    std::lock_guard lock{m_mutex};
    m_quit = true;
  }
  m_cv.notify_all();
  m_thread.join();
  if (m_file)
    std::fclose(m_file);
  m_file = nullptr;
}

std::vector<muse::audio::samples_t>
OfflineAudioDriver::availableOutputDeviceBufferSizes() const
{
  return {m_spec.output.samplesPerChannel};
}

std::vector<muse::audio::sample_rate_t>
OfflineAudioDriver::availableOutputDeviceSampleRates() const
{
  return {m_spec.output.sampleRate};
}

muse::audio::AudioDeviceList OfflineAudioDriver::availableOutputDevices() const
{
  return {{defaultDevice(), "Offline"}};
}

void OfflineAudioDriver::scheduleNextBlock()
{
  const auto due =
      m_start + nanoseconds{m_blocks * m_spec.output.samplesPerChannel *
                            1000000000LL / m_spec.output.sampleRate};
  QTimer::singleShot(std::max(nanoseconds{0}, due - steady_clock::now()),
                     Qt::PreciseTimer, qApp,
                     [this]
                     {
                       if (!m_opened)
                         return;
                       renderBlock();
                       ++m_blocks;
                       scheduleNextBlock();
                     });
}

void OfflineAudioDriver::renderBlock()
{
  std::unique_lock lock{m_mutex};
  m_requested = true;
  m_done = false;
  m_cv.notify_all();
  m_cv.wait(lock, [this] { return m_done; });
  if (!m_file)
    return;
  std::fwrite(m_buffer.data(), sizeof(float), m_buffer.size(), m_file);
  // Read while still open, when the video is finished
  std::fflush(m_file);
}

void OfflineAudioDriver::engineThread()
{
  const std::thread::id thisThread = std::this_thread::get_id();
  while (true)
  {
    {
      std::unique_lock lock{m_mutex};
      m_cv.wait(lock, [this] { return m_requested || m_quit; });
      if (m_quit)
        return;
      m_requested = false;
    }
    // What the engine's worker does before each block in hybrid mode
    muse::async::processMessages(thisThread);
    m_spec.callback(reinterpret_cast<uint8_t *>(m_buffer.data()),
                    int(m_buffer.size() * sizeof(float)));
    {
      std::lock_guard lock{m_mutex};
      m_done = true;
    }
    m_cv.notify_all();
  }
}

muse::audio::IAudioDriverPtr
OfflineAudioDriverFactory::createDriver(const std::string &) const
{
  return std::make_shared<OfflineAudioDriver>();
}
} // namespace dgk
