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
#include "SwipeGestureController.h"
#include <log.h>

#include <array>

namespace dgk
{
namespace
{
constexpr auto leftNote = 59;
constexpr auto rightNote = 60;

void logContacts(const Contacts &contacts)
{
  for (const auto &contact : contacts)
  {
    // LOGD() << "y:" << contact.y << ", t:" << contact.scanTime;
  }
}
} // namespace

SwipeGestureController::SwipeGestureController(const ITouchpad &touchpad)
{
  touchpad.contactChanged().onReceive(
      this,
      [this](const Contacts &contacts)
      {
        logContacts(contacts);
        std::array<std::optional<Finger> *, 2> fingers{{&m_left, &m_right}};
        for (std::optional<Finger> *pFinger : fingers)
        {
          auto &finger = *pFinger;
          if (finger.has_value() &&
              std::find_if(
                  contacts.begin(), contacts.end(), [&](const Contact &contact)
                  { return contact.uid == finger->id; }) == contacts.end())
          {
            // LOGD() << "Lost contact";
            if (finger->triggeringDirection.has_value())
              m_noteOff.send(&finger == &m_left ? leftNote : rightNote);

            finger.reset();
          }
        }

        for (const auto &contact : contacts)
        {
          enum class Side
          {
            None,
            Left,
            Right
          };
          auto side = Side::None;
          if (m_left.has_value() && m_left->id == contact.uid)
            side = Side::Left;
          else if (m_right.has_value() && m_right->id == contact.uid)
            side = Side::Right;
          else
          {
            const auto isLeft = contact.x < 0.5f;
            // LOGD() << "New contact";
            auto &finger = isLeft ? m_left : m_right;
            finger.emplace(contact.uid, contact.y);
          }

          if (side == Side::None)
            continue;

          auto &finger = side == Side::Left ? m_left : m_right;

          // Average time period between samples measured in our experiments.
          constexpr auto samplePeriod = 0.006846681922196797f;
          // Obtained by tuning
          constexpr auto threshold = 0.26390520554812824f * samplePeriod;
          const auto dy = contact.y - finger->prevY;
          finger->prevY = contact.y;

          if (std::abs(dy) > threshold)
          {
            if (!finger->swiping)
            {
              // LOGD() << "Start swiping";
              finger->swiping = true;
              finger->swipeStartY = contact.y;
            }
            ++finger->swipingFor;
          }
          else
          {
            // LOGD() << "Stop swiping";
            const auto direction = dy > 0 ? Direction::up : Direction::down;
            if (finger->swiping && finger->swipingFor >= 7 &&
                finger->triggeringDirection != direction)
            {
              // LOGD() << "Triggering note";
              const auto amplitude = std::abs(contact.y - finger->swipeStartY);
              assert(amplitude <= 1.0f);
              m_noteOn.send(side == Side::Left ? leftNote : rightNote,
                            amplitude);
              finger->triggeringDirection = direction;
            }
            finger->swiping = false;
            finger->swipingFor = 0;
          }
        }
      });
}

muse::async::Channel<int, std::optional<float>>
SwipeGestureController::noteOn() const
{
  return m_noteOn;
}

muse::async::Channel<int> SwipeGestureController::noteOff() const
{
  return m_noteOff;
}
} // namespace dgk