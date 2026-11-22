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
#include <gtest/gtest.h>

#include "internal/VideoClock.h"

namespace dgk
{
namespace
{
using namespace std::chrono_literals;
const VideoClock::TimePoint t0{100s};

TEST(VideoClockTests, OldTimesToldDontPullItBack)
{
  VideoClock clock;
  // Playing from 10 s; told every 100 ms, but the time told is only
  // refreshed every 400 ms.
  for (int i = 0; i <= 20; ++i)
  {
    const auto when = t0 + i * 100ms;
    const double refreshed = 10.0 + (i / 4) * 0.4;
    const double old = clock.Tell(refreshed, when, 1.0, true);
    EXPECT_NEAR(old, (i % 4) * 0.1, 1e-9) << i;
    EXPECT_NEAR(clock.SecondsAt(when), 10.0 + i * 0.1, 1e-9) << i;
  }
}

TEST(VideoClockTests, StartsAfreshWhenMovedOrPaused)
{
  VideoClock clock;
  clock.Tell(10.0, t0, 1.0, true);
  // Sought back by two seconds.
  EXPECT_EQ(clock.Tell(9.0, t0 + 1s, 1.0, true), 0.0);
  EXPECT_DOUBLE_EQ(clock.SecondsAt(t0 + 2s), 10.0);
  // Paused, it stays.
  clock.Tell(10.5, t0 + 3s, 1.0, false);
  EXPECT_FALSE(clock.Playing());
  EXPECT_DOUBLE_EQ(clock.SecondsAt(t0 + 10s), 10.5);
  // At half speed.
  clock.Tell(10.5, t0 + 11s, 0.5, true);
  EXPECT_DOUBLE_EQ(clock.SecondsAt(t0 + 13s), 11.5);
  EXPECT_DOUBLE_EQ(clock.Rate(), 0.5);
}
} // namespace
} // namespace dgk
