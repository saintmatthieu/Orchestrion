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

// Steps shared by the scripts: commands, which respond once done (no sleeping
// for a guessed amount of time), and fail the test case if they don't succeed.

function command(uri, params) {
    var ret = api.commands.dispatch(uri, params || {})
    if (!ret.success) {
        api.testflow.fatal(uri + " failed (code " + ret.code + "): " + ret.text)
    }
    return ret
}

// A path relative to the bundled scores, or an absolute one
function openScore(path) {
    command("command://orchestrion/open-score", { path: path })
}

function toggleFullScreen() {
    command("command://orchestrion/fullscreen")
}

function togglePlay() {
    command("command://orchestrion/play-toggle")
}

function rewind() {
    command("command://orchestrion/rewind")
}

function toggleLoop() {
    command("command://playback/loop-toggle")
}

// The demo cursor glides to a control, by its navigation path (e.g.
// "Controls/Playback/Play"), and clicks it as a user would: returns once
// released.
function click(control) {
    command("command://orchestrion/demo-click", { control: control })
}

// The demo cursor glides to a control and rests there
function moveTo(control) {
    command("command://orchestrion/demo-move", { control: control })
}

function hideCursor() {
    command("command://orchestrion/demo-hide")
}

// The number keys 1 to 0 on screen, going down as they are pressed
function showKeys(shown) {
    command("command://orchestrion/demo-keys", { shown: shown !== false })
}

// An empty caption hides it
function caption(text) {
    command("command://orchestrion/demo-caption", { text: text || "" })
}

// A number key, "0" to "9", as the computer keyboard sends it
function keyDown(key) {
    command("command://orchestrion/demo-key-press", { key: key })
}

function keyUp(key) {
    command("command://orchestrion/demo-key-release", { key: key })
}

function tap(key, ms) {
    keyDown(key)
    api.testflow.sleep(ms)
    keyUp(key)
}

// Returns once the player has stopped, e.g. at the end of the score
function waitUntilStopped() {
    api.commands.dispatch("command://orchestrion/wait-until-stopped", {}, 10 * 60 * 1000)
}

module.exports = {
    command: command,
    openScore: openScore,
    toggleFullScreen: toggleFullScreen,
    togglePlay: togglePlay,
    rewind: rewind,
    toggleLoop: toggleLoop,
    click: click,
    moveTo: moveTo,
    hideCursor: hideCursor,
    showKeys: showKeys,
    caption: caption,
    keyDown: keyDown,
    keyUp: keyUp,
    tap: tap,
    waitUntilStopped: waitUntilStopped
}
