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

#include <QString>

namespace dgk
{
/**
 * Renders a testflow script's video offline (--test-case-render): the
 * application runs on a virtual clock instead of the wall clock, and its
 * frames and sound are rendered on that clock, so that a video doesn't depend
 * on how fast the machine is, nor on how busy: two renders are identical.
 *
 * The virtual time only moves when the main thread has nothing left to do,
 * then straight to when the next timer is due: Qt's timers (C++ and QML), the
 * animations, and the main thread's clocks (QElapsedTimer, std::chrono, ...)
 * all go by it. The window is drawn offscreen and grabbed every 1/30 s of it;
 * the audio engine renders its blocks when that clock says, while the main
 * thread waits (OrchestrionSynthesis' OfflineAudioDriver). Linux only.
 */
class OfflineRenderer
{
public:
  /**
   * Before the application is created: the offscreen platform, the virtual
   * clock.
   */
  static void prepare(const QString &videoPath);

  static bool active();

  /**
   * After the application is created.
   */
  static void start();

  /**
   * The video starts now (once).
   */
  static void startCapture();

  /**
   * Ends the video, with its sound. Blocks until it is written.
   */
  static void finish();
};
} // namespace dgk
