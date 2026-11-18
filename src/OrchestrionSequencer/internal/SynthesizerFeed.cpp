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
#include "SynthesizerFeed.h"

#include <algorithm>

namespace dgk
{
namespace
{
/**
 * Room for many seconds of playing: the audio thread reads its ring every
 * block. It fills only while nothing is rendered (no output device).
 */
constexpr std::size_t ringCapacity = 4096;
} // namespace

void SynthesizerFeed::send(const EventVariant &event)
{
  std::scoped_lock lock{m_mutex};
  // Those let go of are forgotten as the events come
  std::erase_if(m_rings,
                [&](const std::weak_ptr<EventRing> &weak)
                {
                  const std::shared_ptr<EventRing> ring = weak.lock();
                  if (!ring)
                    return true;
                  ring->push(event);
                  return false;
                });
}

std::shared_ptr<EventRing> SynthesizerFeed::connect()
{
  auto ring = std::make_shared<EventRing>(ringCapacity);
  std::scoped_lock lock{m_mutex};
  m_rings.push_back(ring);
  return ring;
}
} // namespace dgk
