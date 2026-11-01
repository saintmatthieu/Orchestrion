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

import Orchestrion 1.0
import Orchestrion.OrchestrionShell 1.0

/**
 * The virtual MIDI keyboard, drawn while it is the MIDI device in use: a
 * piano's 88 keys, A0 to C8. They go down, lit the brighter the harder they
 * were struck, whichever device struck them; clicked, they play the virtual
 * keyboard, the harder the nearer the front of the key. Braces over them,
 * which come and go with the transport, say what their ranges play.
 */
Item {
    id: root

    readonly property int lowest: 21  // A0
    readonly property int highest: 108 // C8
    readonly property int whiteCount: 52
    // Small: they can be clicked, but a mouse is no way to play
    readonly property real whiteWidth: Math.min(24, width / whiteCount)
    readonly property real whiteHeight: whiteWidth * 4.6
    readonly property real blackWidth: whiteWidth * 0.6
    readonly property real blackHeight: whiteHeight * 0.62

    /** Whether the braces over the keys show: as the transport does. */
    property bool rangesShown: true

    // The braces, their labels, and air
    readonly property real rangesHeight: 34

    implicitHeight: rangesHeight + whiteHeight

    VirtualKeyboardModel {
        id: keyboardModel
    }

    Component.onCompleted: keyboardModel.load()

    visible: keyboardModel.shown

    function isBlack(pitch) {
        return [1, 3, 6, 8, 10].indexOf(pitch % 12) >= 0
    }
    // The white keys left of a key's left edge
    function whitesBefore(pitch) {
        let count = 0
        for (let p = lowest; p < pitch; ++p) {
            if (!isBlack(p)) {
                ++count
            }
        }
        return count
    }
    function keyX(pitch) {
        return isBlack(pitch) ? whitesBefore(pitch) * whiteWidth - blackWidth / 2
                              : whitesBefore(pitch) * whiteWidth
    }
    // A key's colour, struck at `velocity` (0 up): lit with the metal of the
    // transport, the brighter the harder
    function keyColor(base, velocity) {
        if (velocity <= 0) {
            return base
        }
        const lit = Theme.metalBright
        return Qt.tint(base, Qt.rgba(lit.r, lit.g, lit.b, 0.35 + 0.65 * velocity))
    }
    // What is under a point: the black keys are over the white ones
    function pitchAt(x, y) {
        for (let p = lowest; p <= highest; ++p) {
            if (isBlack(p) && y < blackHeight && x >= keyX(p) && x < keyX(p) + blackWidth) {
                return p
            }
        }
        for (let p = lowest; p <= highest; ++p) {
            if (!isBlack(p) && x >= keyX(p) && x < keyX(p) + whiteWidth) {
                return p
            }
        }
        return -1
    }

    // The right edge of a key
    function keyRight(pitch) {
        return keyX(pitch) + (isBlack(pitch) ? blackWidth : whiteWidth)
    }
    function rangeLabel(role) {
        switch (role) {
        case "left": return qsTr("Left hand")
        case "right": return qsTr("Right hand")
        case "notes": return qsTr("Notes")
        }
        return ""
    }

    // What the keys play: a brace over each range, its tip up, under a label
    Item {
        id: ranges
        anchors.horizontalCenter: parent.horizontalCenter
        width: keys.width
        height: root.rangesHeight
        opacity: root.rangesShown ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 250 } }

        Repeater {
            model: keyboardModel.ranges

            Item {
                required property var modelData
                // Not quite to the next range's brace
                readonly property real gap: root.whiteWidth * 0.15
                x: root.keyX(modelData.from) + gap
                width: root.keyRight(modelData.to) - root.keyX(modelData.from) - 2 * gap
                height: parent.height

                Text {
                    id: label
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.rangeLabel(modelData.role)
                    // The ink on the parchment: the band is as pale
                    color: Theme.accentInk
                    font.pixelSize: 13
                }

                Canvas {
                    id: brace
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 3
                    height: 9
                    onWidthChanged: requestPaint()
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.reset()
                        const w = width, h = height, mid = w / 2
                        // The curls' radius, and the line's height
                        const r = Math.min(h / 2, w / 4), y = h / 2
                        ctx.strokeStyle = Theme.accentInk
                        ctx.lineWidth = 1.5
                        ctx.beginPath()
                        ctx.moveTo(0, h)
                        ctx.quadraticCurveTo(0, y, r, y)
                        ctx.lineTo(mid - r, y)
                        ctx.quadraticCurveTo(mid, y, mid, 0)
                        ctx.quadraticCurveTo(mid, y, mid + r, y)
                        ctx.lineTo(w - r, y)
                        ctx.quadraticCurveTo(w, y, w, h)
                        ctx.stroke()
                    }
                }
            }
        }
    }

    Item {
        id: keys
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: ranges.bottom
        width: root.whiteCount * root.whiteWidth
        height: root.whiteHeight

        Repeater {
            model: root.highest - root.lowest + 1

            Rectangle {
                readonly property int pitch: root.lowest + index
                readonly property real velocity: keyboardModel.velocities[pitch] || 0
                visible: !root.isBlack(pitch)
                x: root.keyX(pitch)
                width: root.whiteWidth
                height: root.whiteHeight
                radius: 2
                color: root.keyColor(Theme.accent, velocity)
                border.color: Theme.metal
                border.width: 1

            }
        }

        Repeater {
            model: root.highest - root.lowest + 1

            Rectangle {
                readonly property int pitch: root.lowest + index
                readonly property real velocity: keyboardModel.velocities[pitch] || 0
                visible: root.isBlack(pitch)
                x: root.keyX(pitch)
                width: root.blackWidth
                height: root.blackHeight
                radius: 2
                color: root.keyColor(Qt.darker(Theme.backdrop, 1.6), velocity)
                border.color: Theme.metal
                border.width: 1
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            // The key held down, if any
            property int pitch: -1

            function strike(mouse) {
                const pitch = root.pitchAt(mouse.x, mouse.y)
                if (pitch === mouseArea.pitch) {
                    return
                }
                if (mouseArea.pitch >= 0) {
                    keyboardModel.release(mouseArea.pitch)
                }
                mouseArea.pitch = pitch
                if (pitch >= 0) {
                    const depth = root.isBlack(pitch) ? root.blackHeight : root.whiteHeight
                    keyboardModel.press(pitch, 0.01 + 0.99 * Math.min(1, mouse.y / depth))
                }
            }

            onPressed: function(mouse) { strike(mouse) }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    strike(mouse)
                }
            }
            onReleased: {
                if (pitch >= 0) {
                    keyboardModel.release(pitch)
                }
                pitch = -1
            }
        }
    }
}
