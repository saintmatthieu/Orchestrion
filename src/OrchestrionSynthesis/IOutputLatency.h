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

#include <async/notification.h>
#include <modularity/imoduleinterface.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dgk
{
/**
 * The latency dialog (Audio/MIDI > Latency…).
 */
inline constexpr const char *latencyDialogUri = "orchestrion://audio/latency";

/**
 * The audio output's latency against its safety: the margin the driver keeps
 * queued for the device (a level, each adding latency), chosen automatically
 * (raised after an underrun or a close call while something sounds, lowered
 * after long stretches with headroom to spare) or by the user, and what the
 * driver measures of it. Levels change only while the app isn't playing (see
 * OutputLatency).
 *
 * Only where the driver can tell (muse::audio::IAudioOutputMonitor: WASAPI);
 * elsewhere `available` is false.
 */
class IOutputLatency : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IOutputLatency);

public:
  virtual ~IOutputLatency() = default;

  struct Status
  {
    bool available = false;
    std::string driver;
    double periodMs = 0.0;
    /**
     * Each level's margin (queued for the device), in ms, least first.
     */
    std::vector<double> levelsMarginMs;
    bool automatic = false;
    /**
     * The level chosen (by the automatic setting, or the user), and the one in
     * effect: they differ until the output is silent.
     */
    size_t level = 0;
    size_t appliedLevel = 0;

    /**
     * Over the last 10 seconds: how much earlier than needed the callbacks'
     * frames were ready (mean, least), and the rendering's share of the
     * period (mean, most).
     */
    double headroomMeanMs = 0.0;
    double headroomMinMs = 0.0;
    double loadMeanPercent = 0.0;
    double loadMaxPercent = 0.0;

    /**
     * Since the counters were reset. Underruns: callbacks that came too late,
     * Orchestrion's doing. Clock stalls: the device's position stopped (silence
     * played), whoever was late.
     */
    uint64_t underruns = 0;
    uint64_t clockStalls = 0;
    double secondsSinceLastUnderrun = -1.0; // -1: none
    /**
     * The last two minutes, a second each, oldest first.
     */
    std::vector<int> underrunsPerSecond;
    std::vector<int> clockStallsPerSecond;
  };

  virtual Status status() const = 0;
  /**
   * Several times a second, while the output runs.
   */
  virtual muse::async::Notification statusChanged() const = 0;

  virtual void setAutomatic(bool automatic) = 0;
  /**
   * Taken as the user's choice: the automatic setting is switched off.
   */
  virtual void setLevel(size_t level) = 0;
  /**
   * The counters and history, and the automatic setting's level: it starts
   * again from the least margin.
   */
  virtual void resetCounters() = 0;

  /**
   * For trying the detection: the next callback starts so much later.
   */
  virtual void delayNextCallback(double ms) = 0;

  /**
   * The hint offering the latency dialog ("Audio glitch?"): shown after an
   * underrun while something sounds and the level is the user's (the
   * automatic setting deals with it otherwise), until closed: back at the next
   * such underrun, or, closed for good, never again.
   */
  virtual bool glitchHintShown() const = 0;
  virtual muse::async::Notification glitchHintChanged() const = 0;
  virtual void closeGlitchHint(bool forGood) = 0;
};
} // namespace dgk
