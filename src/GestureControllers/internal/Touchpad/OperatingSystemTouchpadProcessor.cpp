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
#include "OperatingSystemTouchpadProcessor.h"
#include "log.h"
#include <QCoreApplication>

#ifdef OS_IS_WIN
#include <windows.h>
#endif

namespace dgk
{
OperatingSystemTouchpadProcessor::OperatingSystemTouchpadProcessor(std::unique_ptr<IClock> clock)
    : m_clock{std::move(clock)},
      m_timeoutThread{&OperatingSystemTouchpadProcessor::timeoutThreadFunc, this}
{
  #ifdef OS_IS_WIN
  SetThreadPriority(m_timeoutThread.native_handle(),
                    THREAD_PRIORITY_ABOVE_NORMAL);
  #endif
}

namespace
{
static auto uid = 0;
void ScanToContacts(const TouchpadScan &scan, Contacts &contacts,
                    std::unordered_map<int, ContactInfo> &uuidMap)
{
  contacts.reserve(scan.contacts.size());
  for (const auto &contact : scan.contacts)
  {
    if (uuidMap.count(contact.number) == 0)
      uuidMap.emplace(contact.number, ContactInfo{uid++, scan.scanTime});

    auto &contactInfo = uuidMap.at(contact.number);
    const auto msDiff = scan.scanTime - contactInfo.lastScanTime;
    if (msDiff > std::chrono::milliseconds{20})
      contactInfo.uid = uid++;

    contactInfo.lastScanTime = scan.scanTime;
    contacts.emplace_back(contactInfo.uid, contact.x, contact.y);
  }
}
} // namespace

void OperatingSystemTouchpadProcessor::timeoutThreadFunc()
{
  Contacts contacts;
  contacts.reserve(5);
  m_uidMap.reserve(5);
  while (true)
  {
    {
      std::unique_lock lock{m_mutex};

      if (!m_lastContactTime.has_value())
        m_cv.wait(lock, [this] { return !m_scanQueue.empty() || !m_running; });
      else
        m_cv.wait_until(lock,
                        *m_lastContactTime + std::chrono::milliseconds{150},
                        [this] { return !m_scanQueue.empty() || !m_running; });

      if (!m_running)
        return;

      if (m_scanQueue.empty())
      {
        // timed out
        m_uidMap.clear();
        m_lastContactTime.reset();
      }
      else
      {
        const auto &scan = m_scanQueue.front();
        ScanToContacts(scan, contacts, m_uidMap);
        m_scanQueue.pop();
      }
    }

    m_contactChanged.send(contacts);
    contacts.clear();
  }
}

OperatingSystemTouchpadProcessor::~OperatingSystemTouchpadProcessor()
{
  m_running = false;
  m_cv.notify_all();
  m_timeoutThread.join();
}

void OperatingSystemTouchpadProcessor::process(TouchpadScan scan)
{
  auto now = m_clock->now();
  {
    std::unique_lock lock{m_mutex};
    m_lastContactTime = std::move(now);
    m_scanQueue.push(std::move(scan));
  }
  m_cv.notify_one();
}

muse::async::Channel<Contacts> OperatingSystemTouchpadProcessor::contactChanged() const
{
  return m_contactChanged;
}

} // namespace dgk