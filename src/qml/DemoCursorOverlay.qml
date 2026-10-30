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
 * Draws the demo cursor (DemoCursor) over the whole window: a pointer, a ring
 * around its tip that makes it easy to follow, and a ripple at each click. It
 * takes no input: the clicks are DemoCursor's mouse events.
 */
Item {
    id: root

    // The theme's glow, the colour of a lit button, edged with its backdrop's
    // mahogany: the one shows on the dark lining, the other on the parchment.
    readonly property color gold: Theme.metalBright
    readonly property color edge: Theme.backdrop

    enabled: false

    // A gold band edged in mahogany inside and out, centred on its position
    component EdgedRing: Item {
        id: ring
        property real diameter: 44
        property real band: 2.5
        property real fill: 0
        width: diameter
        height: diameter
        x: -width / 2
        y: -height / 2

        Rectangle {
            anchors.fill: parent
            anchors.margins: -1.5
            radius: width / 2
            color: "transparent"
            border.color: root.edge
            border.width: 1.5
        }
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: Qt.rgba(root.gold.r, root.gold.g, root.gold.b, ring.fill)
            border.color: root.gold
            border.width: ring.band
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: ring.band
            radius: width / 2
            color: "transparent"
            border.color: root.edge
            border.width: 1.5
        }
    }

    Item {
        id: tip

        // DemoCursor's position is the window's, which this item may not
        // start at.
        readonly property point local: root.mapFromItem(null, DemoCursor.position.x,
                                                        DemoCursor.position.y)
        x: local.x
        y: local.y

        opacity: DemoCursor.shown ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 200 } }

        EdgedRing {
            fill: 0.2
            scale: DemoCursor.pressed ? 0.75 : 1
            Behavior on scale { NumberAnimation { duration: 90 } }
        }

        EdgedRing {
            id: ripple
            band: 3
            opacity: 0
            scale: 1

            ParallelAnimation {
                id: rippleAnimation
                NumberAnimation { target: ripple; property: "scale"; from: 0.8; to: 2.2; duration: 450; easing.type: Easing.OutCubic }
                NumberAnimation { target: ripple; property: "opacity"; from: 0.9; to: 0; duration: 450; easing.type: Easing.InQuad }
            }
        }

        Connections {
            target: DemoCursor
            function onPressedChanged() {
                if (DemoCursor.pressed) {
                    rippleAnimation.restart()
                }
            }
        }

        // A plain arrow, its tip on the position, white edged with black so
        // that it shows on the parchment and on the dark lining alike.
        Canvas {
            width: 24
            height: 34
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.beginPath()
                ctx.moveTo(1.5, 1.5)
                ctx.lineTo(1.5, 26)
                ctx.lineTo(7.5, 20.5)
                ctx.lineTo(11.5, 30)
                ctx.lineTo(15.5, 28.5)
                ctx.lineTo(11.5, 19.5)
                ctx.lineTo(19.5, 19.5)
                ctx.closePath()
                ctx.fillStyle = "white"
                ctx.strokeStyle = "black"
                ctx.lineWidth = 1.5
                ctx.lineJoin = "round"
                ctx.fill()
                ctx.stroke()
            }
        }
    }
}
