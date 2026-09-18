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

/**
 * The backdrop's outermost lining: a rectangle on the view's own border, in the
 * theme's metal and cut with the same pen as the glyphs, so it carries the same
 * weight as the case around the MIDI keyboard indicator — 1.46 px on the
 * uprights, 1.10 px on the horizontals (tools/gen_player_glyphs.py, stroke_w at
 * 90 and 0 degrees). Four sides rather than one Rectangle's border, which can
 * only be one width all the way round and would lose that contrast.
 *
 * It takes the view's bounds rather than the window's, which is what makes it
 * land on the edge of the screen in full screen and run just under the menu bar
 * in a window. The corner icons are placed in the same coordinates, so their
 * inset from this line is the same in both.
 */
Item {
    id: root

    readonly property color metal: Theme.metal
    readonly property real upright: 1.46
    readonly property real horizontal: 1.10

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.upright
        color: root.metal
        antialiasing: true
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.upright
        color: root.metal
        antialiasing: true
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.horizontal
        color: root.metal
        antialiasing: true
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: root.horizontal
        color: root.metal
        antialiasing: true
    }
}
