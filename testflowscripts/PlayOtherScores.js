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
 * Play other scores: the title opens the score shelf, which goes on down to
 * the library, and a search there opens a score of Rolls. The app gives the
 * user's score back after it (Tutorials).
 *
 * @testflow record-from The title
 */

var Orchestrion = require("steps/Orchestrion.js")

var testCase = {
    name: "Play other scores",
    description: "Where to find scores, and how to open them",
    steps: [
        {name: "The title", func: function() {
            Orchestrion.caption("To find scores, click on the title.")
            api.testflow.sleep(1500)
            Orchestrion.click("Title")
            Orchestrion.hideCursor()
            api.testflow.sleep(1000)
        }},
        {name: "The library", func: function() {
            Orchestrion.caption("You can find all scores available on rolls.orchestrion.app, and more.")
            api.testflow.sleep(1000)
            Orchestrion.moveTo("Shelf/Cards")
            for (var i = 0; i < 50; ++i) {
                Orchestrion.scroll(-16)
                api.testflow.sleep(33)
            }
            Orchestrion.hideCursor()
            api.testflow.sleep(500)
        }},
        {name: "Search", func: function() {
            Orchestrion.caption("Find your score easily with the search box")
            Orchestrion.click("Shelf/Search")
            api.testflow.sleep(300)
            Orchestrion.type("beethov")
            api.testflow.sleep(1500)
            Orchestrion.click("Shelf/Card")
            Orchestrion.waitUntilGone("Shelf/Cards")
            Orchestrion.caption("")
            Orchestrion.hideCursor()
            api.testflow.sleep(1000)
        }},
        {name: "The catalogue", func: function() {
            Orchestrion.caption("You can consult our catalogue on rolls.orchestrion.app. Happy browsing!")
            api.testflow.sleep(6000)
        }},
    ]
}

function main() {
    api.testflow.runTestCase(testCase)
}
