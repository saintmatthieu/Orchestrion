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
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import Qt5Compat.GraphicalEffects

import Orchestrion 1.0

/**
 * One playback control: a glyph cut from the score title's typeface (see
 * tools/gen_player_glyphs.py), painted in the theme's metal and glowing to show
 * its state.
 *
 * Plain metal means ready, glowing means doing, and there is no second glyph
 * for any state: play stays a triangle and lights up while it plays, exactly as
 * loop lights up while looping.
 *
 * The glow's strength is never animated directly. It is a base, which rises
 * when the button is engaged or held, plus an amplitude, which rises while the
 * pointer rests on an idle button, times the phase shared by the whole row.
 * Both targets are eased by a Behavior, so every state change eases from
 * wherever the level currently stands and it can never jump.
 */
Item {
    id: root

    /** File name of the glyph in qrc:/icons/player/, e.g. "play.svg". */
    property string icon

    /** The action this button stands for is currently running. */
    property bool engaged: false

    property string tooltip

    signal clicked()

    width: 36
    height: 36

    readonly property real level: Math.min(1, base + amp * TransportGlow.phase)

    property real base: (root.engaged || mouseArea.pressed) ? 1 : 0
    property real amp: (mouseArea.containsMouse && !root.engaged && !mouseArea.pressed)
                       ? 0.85 : 0

    Behavior on base { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
    Behavior on amp { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }

    // The shared oscillator only ticks while a button has use for it, so an
    // untouched row costs nothing. Counted here rather than from `containsMouse`
    // so that the phase keeps running through the half-second fade-out.
    readonly property bool usesPhase: amp > 0.001
    onUsesPhaseChanged: TransportGlow.users += usesPhase ? 1 : -1
    Component.onDestruction: {
        if (usesPhase) {
            TransportGlow.users -= 1
        }
    }

    Image {
        id: iconSource
        anchors.fill: parent
        source: "qrc:/icons/player/" + root.icon
        // Rasterize the drawing at device resolution rather than at the item's
        // logical size, which would leave the hairlines soft on a HiDPI screen.
        sourceSize: Qt.size(root.width * Screen.devicePixelRatio,
                            root.height * Screen.devicePixelRatio)
        fillMode: Image.PreserveAspectFit
        mipmap: true
        visible: false
    }

    // Drawn first, so the halo sits behind the glyph. Glow paints the source's
    // alpha in its own colour, so it does not matter that the source is untinted.
    Glow {
        anchors.fill: iconSource
        source: iconSource
        color: Theme.metalBright
        radius: 5
        samples: 11
        spread: 0.1 + 0.45 * root.level
        opacity: root.level
        transparentBorder: true
        visible: opacity > 0
    }

    ColorOverlay {
        anchors.fill: iconSource
        source: iconSource
        // The theme's metal at rest, warming towards the halo's colour as the
        // glow rises.
        color: Qt.rgba(Theme.metal.r + (Theme.metalBright.r - Theme.metal.r) * root.level,
                       Theme.metal.g + (Theme.metalBright.g - Theme.metal.g) * root.level,
                       Theme.metal.b + (Theme.metalBright.b - Theme.metal.b) * root.level,
                       1)
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    ToolTip {
        delay: 500
        visible: mouseArea.containsMouse && root.tooltip.length > 0
        text: root.tooltip
    }
}
