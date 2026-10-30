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

module.exports = {
    command: command,
    openScore: openScore,
    toggleFullScreen: toggleFullScreen,
    togglePlay: togglePlay,
    rewind: rewind,
    toggleLoop: toggleLoop
}
