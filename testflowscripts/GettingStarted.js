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
 * Getting started: playing the open score with the computer keyboard's number
  * keys - a key of each hand's half, short and held notes, then the whole
 * score at speed, from the top.
 *
 * @testflow record-after Full screen
 */

var Orchestrion = require("steps/Orchestrion.js")

// The keys each hand uses here, as a player's middle and index fingers would
var LEFT = "3"
var RIGHT = ["8", "9"]

var testCase = {
    name: "Getting started",
    description: "Plays the open score with the number keys",
    steps: [
        {name: "Full screen", func: function() {
            Orchestrion.toggleFullScreen()
        }},
        {name: "Numbers play notes and chords", func: function() {
            Orchestrion.showKeys()
            Orchestrion.caption("Numbers play notes and chords")
            api.testflow.sleep(2500)
        }},
        {name: "The left hand", func: function() {
            Orchestrion.tap(LEFT, 900)
            api.testflow.sleep(600)
        }},
        {name: "The right hand", func: function() {
            Orchestrion.tap(RIGHT[0], 900)
            api.testflow.sleep(600)
        }},
        {name: "Short notes", func: function() {
            for (var i = 0; i < 4; ++i) {
                Orchestrion.tap(RIGHT[i % 2], 110)
                api.testflow.sleep(450)
            }
        }},
        {name: "Held notes", func: function() {
            // Each key goes down before the previous one comes up
            Orchestrion.keyDown(RIGHT[0])
            for (var i = 1; i < 5; ++i) {
                api.testflow.sleep(600)
                Orchestrion.keyDown(RIGHT[i % 2])
                api.testflow.sleep(40)
                Orchestrion.keyUp(RIGHT[(i + 1) % 2])
            }
            api.testflow.sleep(600)
            Orchestrion.keyUp(RIGHT[0])
        }},
        {name: "And now play at speed", func: function() {
            Orchestrion.caption("And now play at speed")
                        // From the top: the hands are ahead of where the player would
            // start, which it would first wait to catch up with.
            Orchestrion.rewind()
            api.testflow.sleep(1500)
            Orchestrion.togglePlay()
            Orchestrion.waitUntilStopped()
            Orchestrion.caption("")
            Orchestrion.showKeys(false)
            api.testflow.sleep(800)
        }},
    ]
}

function main() {
    api.testflow.runTestCase(testCase)
}
