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

/*
 * Plays a bundled score, then loops it.
 *
 * @testflow record-after Open Frère Jacques
 */

var Orchestrion = require("steps/Orchestrion.js")

var testCase = {
    name: "TC1: Play a score",
    description: "Opens a bundled score and lets the automatic player play a few bars of it",
    steps: [
        {name: "Full screen", func: function() {
            Orchestrion.toggleFullScreen()
        }},
        {name: "Open Frère Jacques", func: function() {
            Orchestrion.openScore("Frere Jacques.mxl")
        }},
        {name: "Play", func: function() {
            Orchestrion.togglePlay()
            api.testflow.sleep(3000)
        }},
        {name: "Stop", func: function() {
            Orchestrion.togglePlay()
        }},
        {name: "Loop and play from the start", func: function() {
            Orchestrion.rewind()
            Orchestrion.toggleLoop()
            Orchestrion.togglePlay()
            api.testflow.sleep(2000)
            Orchestrion.togglePlay()
        }},
    ]
}

function main() {
    api.testflow.runTestCase(testCase)
}
