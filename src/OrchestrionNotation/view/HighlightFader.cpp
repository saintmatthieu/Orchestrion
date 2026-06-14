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
#include "HighlightFader.h"
#include <algorithm>

namespace dgk
{
HighlightFader::HighlightFader(std::function<void()> requestRepaint,
                               qint64 fadeMs)
    : m_requestRepaint(std::move(requestRepaint)), m_fadeMs(fadeMs)
{
  m_clock.start();
  m_timer.setInterval(16); // ~60 fps while fading
  m_timer.callOnTimeout([this] { advance(); });
}

void HighlightFader::add(Highlight highlight)
{
  m_entries.push_back({std::move(highlight), m_clock.elapsed()});
  if (!m_timer.isActive())
    m_timer.start();
}

void HighlightFader::clear()
{
  m_entries.clear();
  m_timer.stop();
}

void HighlightFader::forEach(
    const std::function<void(const Highlight &, double)> &draw) const
{
  const qint64 now = m_clock.elapsed();
  for (const Entry &entry : m_entries)
  {
    const double progress =
        std::clamp(static_cast<double>(now - entry.startMs) /
                       static_cast<double>(m_fadeMs),
                   0.0, 1.0);
    draw(entry.highlight, 1.0 - progress);
  }
}

void HighlightFader::advance()
{
  const qint64 now = m_clock.elapsed();
  m_entries.erase(
      std::remove_if(m_entries.begin(), m_entries.end(),
                     [now, this](const Entry &entry)
                     { return now - entry.startMs >= m_fadeMs; }),
      m_entries.end());
  if (m_entries.empty())
    m_timer.stop();
  m_requestRepaint();
}
} // namespace dgk
