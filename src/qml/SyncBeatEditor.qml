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
import QtQuick 2.15
import QtQuick.Controls 2.15
import Orchestrion 1.0

/**
 * The time of a beat tapped to sync a video with the score, over its marker
 * while that is hovered, and while this is: a spin box to the hundredth of a
 * second moves it, the video replaying from just before it.
 */
Rectangle {
    id: root

    /** A ScoreRecordingsModel. */
    property var model
    /** The OrchestrionNotationPaintView the markers are on. */
    property var paintView
    /** The beat shown; -1 hides it. */
    property int beat: -1
    // Bumped when the beats change, for the bindings on the model's
    // invokables to be evaluated again.
    property int revision: 0

    visible: beat >= 0
    width: column.implicitWidth + 20
    height: column.implicitHeight + 16
    radius: 6
    color: Theme.popup
    border.color: Theme.metal
    border.width: 1

    function close() {
        beat = -1
        spin.focus = false
    }

    function place() {
        const marker = parent.mapFromItem(paintView, paintView.hoveredSyncBeatPos.x,
                                          paintView.hoveredSyncBeatPos.y)
        x = Math.max(0, Math.min(parent.width - width, marker.x - width / 2))
        // Over the parchment: below a marker on its top edge, above one on
        // its bottom edge.
        y = marker.y < parent.height / 2 ? marker.y + 16 : marker.y - height - 16
    }

    Connections {
        target: root.paintView
        function onHoveredSyncBeatChanged() {
            if (root.paintView.hoveredSyncBeat >= 0) {
                closeTimer.stop()
                root.beat = root.paintView.hoveredSyncBeat
                root.place()
            } else {
                closeTimer.restart()
            }
        }
    }

    Connections {
        target: root.model
        function onSyncChanged() {
            root.revision++
            if (!root.model.syncing)
                root.close()
        }
    }

    // From the marker to here, the pointer crosses the score: a moment's
    // grace before closing.
    Timer {
        id: closeTimer
        interval: 400
        onTriggered: {
            if (!hover.hovered && root.paintView.hoveredSyncBeat < 0)
                root.close()
        }
    }

    HoverHandler {
        id: hover
        onHoveredChanged: {
            if (!hovered)
                closeTimer.restart()
        }
    }

    Column {
        id: column
        x: 10
        y: 8
        spacing: 4

        Text {
            text: {
                root.revision
                return root.model.syncBeatName(root.beat)
            }
            color: Theme.metalBright
            font.pixelSize: 13
        }

        SpinBox {
            id: spin
            // Not typed in: Return taps beats.
            editable: false
            stepSize: 1
            // In hundredths of a second, in the video.
            from: {
                root.revision
                return Math.round(root.model.syncBeatMinimum(root.beat) * 100)
            }
            to: {
                root.revision
                return Math.round(root.model.syncBeatMaximum(root.beat) * 100)
            }
            value: {
                root.revision
                return Math.round(root.model.syncBeatSeconds(root.beat) * 100)
            }
            textFromValue: function (value) {
                const minutes = Math.floor(value / 6000)
                const seconds = Math.floor(value / 100) % 60
                const hundredths = value % 100
                return minutes + ":" + String(seconds).padStart(2, "0") + "."
                        + String(hundredths).padStart(2, "0")
            }
            onValueModified: root.model.setSyncBeatSeconds(root.beat, value / 100)
        }
    }
}
