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
 * Automatic accompaniment: the staves' modes, which the left cylinder
 * uncovers; the lower staff stepped through them to Follow, then the upper
 * one played, which the lower follows.
 *
 * @testflow record-from The staves' modes
 */

var Orchestrion = require("steps/Orchestrion.js")

// The staff that accompanies: the lower one, the left hand's
var FOLLOWER = 1
var SELECTOR = "Score/StaffMode/" + FOLLOWER + "/"

// The modes this script changed, to be given back
var edits = 0

var testCase = {
    name: "Automatic accompaniment",
    description: "A staff set to Follow accompanies the other",
    steps: [
        {name: "The staves' modes", func: function() {
            Orchestrion.caption("Each staff has its mode")
            // Which the cylinder rolls in to uncover
            Orchestrion.moveTo("Score/Cylinder")
            api.testflow.sleep(600)
            // Its name, in the tooltip
            Orchestrion.moveTo(SELECTOR + "mode")
            api.testflow.sleep(1500)
        }},
        {name: "Follow", func: function() {
            Orchestrion.caption("Set one to Follow")
            // Round the modes, as many as there are
            for (var i = 0; i < 4 && Orchestrion.staffMode(FOLLOWER) !== "follow"; ++i) {
                Orchestrion.click(SELECTOR + "next")
                ++edits
                Orchestrion.moveTo(SELECTOR + "mode")
                api.testflow.sleep(1000)
            }
            if (Orchestrion.staffMode(FOLLOWER) !== "follow") {
                api.testflow.fatal("the lower staff isn't in the Follow mode")
            }
        }},
        {name: "Play the other staff", func: function() {
            Orchestrion.hideCursor()
            Orchestrion.caption("Play the other: it follows you")
            Orchestrion.showKeys()
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
    for (; edits > 0; --edits) {
        Orchestrion.undo()
    }
}
