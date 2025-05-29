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

#include "../GestureControllerInternalTypes.h"
#include "IClock.h"

#include <async/channel.h>

#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>

namespace dgk
{
struct ContactInfo
{
  ContactInfo(int uid, std::chrono::milliseconds lastScanTime)
      : uid{uid}, lastScanTime{lastScanTime}
  {
  }
  int uid;
  std::chrono::milliseconds lastScanTime;
};

class OperatingSystemTouchpadProcessor
{
public:
  OperatingSystemTouchpadProcessor(std::unique_ptr<IClock> clock);
  ~OperatingSystemTouchpadProcessor();

  void process(TouchpadScan contacts);
  muse::async::Channel<Contacts> contactChanged() const;

private:
  void timeoutThreadFunc();

  using optional_time_point =
      std::optional<std::chrono::time_point<std::chrono::steady_clock>>;

  const std::unique_ptr<IClock> m_clock;
  std::thread m_timeoutThread;
  bool m_running = true;
  muse::async::Channel<Contacts> m_contactChanged;
  std::condition_variable m_cv;
  std::mutex m_mutex;
  std::queue<TouchpadScan> m_scanQueue;
  optional_time_point m_lastContactTime;
  std::unordered_map<int /* raw contact ID */, ContactInfo> m_uidMap;
};
} // namespace dgk