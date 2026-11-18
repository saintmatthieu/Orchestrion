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

#include "ISynthesizerFeed.h"

#include <mutex>
#include <vector>

namespace dgk
{
/**
 * Fans each event out to the connected rings. The mutex is only taken by the
 * sender and by those connecting, never by the audio thread.
 */
class SynthesizerFeed : public ISynthesizerFeed
{
public:
  void send(const EventVariant &) override;
  std::shared_ptr<EventRing> connect() override;

private:
  std::mutex m_mutex;
  std::vector<std::weak_ptr<EventRing>> m_rings;
};
} // namespace dgk
