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

#include "BuiltInEffectRuntime.h"
#include "ReverbPresets.h"

namespace dgk
{
/**
 * Muse Reverb's parameters for each preset, by the names the processor gives
 * them. Tuned by ear on the piano (September 2026): each step roughly doubles
 * the decay, early reflections fade and the tail grows as the space widens,
 * the tone darkens from wood to stone. Whatever isn't listed keeps the
 * processor's default; the dry signal always passes at unity, this being an
 * insert, not an aux send.
 */
inline ReverbParameters reverbPresetParameters(ReverbPreset preset)
{
  ReverbParameters p{{"DryLevel", 0.f}, {"Quality", 4.f}, {"TimeMid", 100.f}};
  switch (preset)
  {
  case ReverbPreset::Room:
    p.insert({{"ReverbTimeMs", 750.f}, {"LateRoomScale", 0.55f},
              {"PreDelay", 8.f},       {"ERDirect", -12.f},
              {"ERtoLate", -20.f},     {"LateLevel", -26.f},
              {"FeedbackTop", 6000.f}, {"TimeLow", 100.f},
              {"TimeHigh", 60.f},      {"LowMidFreq", 300.f},
              {"MidHighFreq", 3500.f}, {"ModAmp", 0.1f},
              {"ModFreq", 0.8f},       {"Stereo", 90.f},
              {"LowCut", 80.f},        {"HighCut", 12000.f}});
    break;
  case ReverbPreset::SmallHall:
    p.insert({{"ReverbTimeMs", 1600.f}, {"LateRoomScale", 1.1f},
              {"PreDelay", 18.f},       {"ERDirect", -16.f},
              {"ERtoLate", -22.f},      {"LateLevel", -26.f},
              {"FeedbackTop", 6500.f},  {"TimeLow", 100.f},
              {"TimeHigh", 55.f},       {"LowMidFreq", 350.f},
              {"MidHighFreq", 4000.f},  {"ModAmp", 0.12f},
              {"ModFreq", 0.7f},        {"Stereo", 100.f},
              {"LowCut", 60.f},         {"HighCut", 13000.f}});
    break;
  case ReverbPreset::LargeHall:
    p.insert({{"ReverbTimeMs", 2300.f}, {"LateRoomScale", 1.6f},
              {"PreDelay", 28.f},       {"ERDirect", -18.f},
              {"ERtoLate", -20.f},      {"LateLevel", -24.f},
              {"FeedbackTop", 7500.f},  {"TimeLow", 105.f},
              {"TimeHigh", 50.f},       {"LowMidFreq", 350.f},
              {"MidHighFreq", 4000.f},  {"ModAmp", 0.15f},
              {"ModFreq", 0.6f},        {"Stereo", 115.f},
              {"LowCut", 60.f},         {"HighCut", 14000.f}});
    break;
  case ReverbPreset::Cathedral:
    p.insert({{"ReverbTimeMs", 5500.f}, {"LateRoomScale", 3.f},
              {"PreDelay", 45.f},       {"ERDirect", -24.f},
              {"ERtoLate", -16.f},      {"LateLevel", -20.f},
              {"FeedbackTop", 4500.f},  {"TimeLow", 125.f},
              {"TimeHigh", 45.f},       {"LowMidFreq", 300.f},
              {"MidHighFreq", 3000.f},  {"ModAmp", 0.2f},
              {"ModFreq", 0.4f},        {"Stereo", 130.f},
              {"LowCut", 50.f},         {"HighCut", 10000.f}});
    break;
  }
  return p;
}
} // namespace dgk
