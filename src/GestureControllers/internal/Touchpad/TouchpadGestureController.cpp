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
#include "TouchpadGestureController.h"
#include <log.h>

namespace dgk
{
namespace
{
constexpr auto middleC = 60;
}

TouchpadGestureController::TouchpadGestureController(const ITouchpad &touchpad)
    : m_touchpad{touchpad}
{
  touchpad.contactChanged().onReceive(
      this,
      [this](const Contacts &contacts)
      {
        const std::vector<int> usedKeys{
            [this]
            {
              std::vector<int> keys;
              std::transform(m_pressedKeys.begin(), m_pressedKeys.end(),
                             std::back_inserter(keys),
                             [](const auto &pair) { return pair.second; });
              return keys;
            }()};
        // detect new contacts:
        for (const auto &contact : contacts)
        {
          if (!m_pressedKeys.count(contact.uid))
          {
            const auto isLeft = contact.x < 0.5f;

            // Instead of the y value, use the lowest y value of the matching
            // hand. This is because, while playing legato, it is challenging to
            // hit the touchpad at the same height with different fingers of one
            // same hand.
            auto y = 1.f;
            for (const auto &c : contacts)
              if (c.x < 0.5f == isLeft)
                y = std::min(y, c.y);

            const auto incr = isLeft ? -1 : 1;
            auto key = isLeft ? middleC - 1 : middleC;
            while (std::find(usedKeys.begin(), usedKeys.end(), key) !=
                   usedKeys.end())
              key += incr;
            m_pressedKeys.emplace(contact.uid, key);
            m_noteOn.send(key, 1 - y);
          }
        }
        // detect removed contacts:
        for (auto it = m_pressedKeys.begin(); it != m_pressedKeys.end();)
        {
          if (std::find_if(
                  contacts.begin(), contacts.end(), [&](const Contact &contact)
                  { return contact.uid == it->first; }) == contacts.end())
          {
            const auto [id, key] = *it;
            it = m_pressedKeys.erase(it);
            m_noteOff.send(key);
          }
          else
            ++it;
        }
      });
}

bool TouchpadGestureController::isFunctional()
{
  return true;
}

muse::async::Channel<Contacts> TouchpadGestureController::contactChanged() const
{
  return m_touchpad.contactChanged();
}

muse::async::Channel<int, float> TouchpadGestureController::noteOn() const
{
  return m_noteOn;
}

muse::async::Channel<int> TouchpadGestureController::noteOff() const
{
  return m_noteOff;
}
} // namespace dgk