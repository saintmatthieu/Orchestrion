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
#pragma once

#include "GestureControllers/GestureControllerTypes.h"
#include "MuseScoreShellTypes.h"
#include <unordered_map>

namespace dgk::actionIds
{
static const std::unordered_map<DeviceType, const char *> chooseDevicesSubmenu =
    {{DeviceType::MidiController, "chooseMidiControllerSubmenu"},
     {DeviceType::MidiSynthesizer, "chooseMidiSynthesizerSubmenu"},
     {DeviceType::PlaybackDevice, "choosePlaybackDeviceSubmenu"}};

static const std::unordered_map<GestureControllerType, const char *>
    toggleController = {
        {GestureControllerType::MidiDevice,
         "orchestrion-toggle-controller-midi"},
        {GestureControllerType::Touchpad,
         "orchestrion-toggle-controller-touchpad"},
        {GestureControllerType::Swipe, "orchestrion-toggle-controller-swipe"},
        {GestureControllerType::ComputerKeyboard,
         "orchestrion-toggle-controller-keyboard"}};

static constexpr const char *loopIn = "orchestrion-loop-in";
static constexpr const char *loopOut = "orchestrion-loop-out";
// Programmatic menu control, e.g. for the welcome tour's example-scores step.
static constexpr const char *openFileMenu = "orchestrion-open-file-menu";
static constexpr const char *closeAppMenu = "orchestrion-close-app-menu";
static constexpr const char *reverbOff = "orchestrion-advanced-reverb-off";
static constexpr const char *reverbRoom = "orchestrion-advanced-reverb-room";
static constexpr const char *reverbHall = "orchestrion-advanced-reverb-hall";
static constexpr const char *reverbCathedral =
    "orchestrion-advanced-reverb-cathedral";

//! Orchestrion's own transport actions. Deliberately NOT MuseScore's
//! "play"/"stop": those ids are also handled — and sometimes dispatched —
//! by MuseScore code with its rendered-track playback in mind (e.g. the
//! preferences dialog dispatches "stop"), while Orchestrion playback is
//! just scheduled gesture events. Own ids keep the two worlds apart.
static constexpr const char *playbackToggle = "orchestrion-play";
static constexpr const char *playbackStop = "orchestrion-stop";

//! Opens the grading settings dialog (from the Grading menu; the top-row
//! settings button opens it directly).
static constexpr const char *gradingSettings = "orchestrion-grading-settings";

//! Auto-play: which hand the machine plays, and the action that opens the
//! choice popup (the top-row button opens it directly).
static constexpr const char *autoPlaySettings = "orchestrion-autoplay-choose";
//! Development menu: whether auto-play / grading are offered at all.
static constexpr const char *toggleAutoPlayExposure =
    "orchestrion-dev-toggle-autoplay-exposure";
static constexpr const char *toggleGradingExposure =
    "orchestrion-dev-toggle-grading-exposure";
static constexpr const char *autoPlayNone = "orchestrion-autoplay-none";
static constexpr const char *autoPlayLeftHand = "orchestrion-autoplay-left";
static constexpr const char *autoPlayRightHand = "orchestrion-autoplay-right";

static constexpr const char *playModePerformance =
    "orchestrion-advanced-play-mode-performance";
static constexpr const char *playModeFittedTempo =
    "orchestrion-advanced-play-mode-fitted-tempo";
static constexpr const char *playModeMetronome =
    "orchestrion-advanced-play-mode-metronome";
} // namespace dgk::actionIds
