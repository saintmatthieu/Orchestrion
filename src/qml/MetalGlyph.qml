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
import Qt5Compat.GraphicalEffects

import Orchestrion 1.0

/**
 * One of the drawings cut from the score title's typeface (see
 * tools/gen_player_glyphs.py), painted in the current theme's metal and
 * glowing to show its state.
 *
 * Plain metal means ready, glowing means doing: a transport button lights up
 * while its action runs, the MIDI indicator while a keyboard is connected.
 * Nothing is ever dimmed, and no glyph is ever swapped for another. The
 * drawings are single-colour and tinted here, so one file serves both themes.
 *
 * The glow's strength is never animated directly. It is a base, which rises
 * when the glyph is engaged or held, plus an amplitude, which rises while the
 * pointer rests on it, times the phase shared by every glyph on screen. Both
 * targets are eased by a Behavior, so a state change eases from wherever the
 * level currently stands and it can never jump.
 */
Item {
    id: root

    property url source

    /** The thing this glyph stands for is currently happening. */
    property bool engaged: false

    property bool hovered: false
    property bool pressed: false

    /**
     * How much halo this drawing takes at full level, where 1.0 is a bloom
     * that swallows the mark it is meant to light.
     *
     * The default is the strongest the set is cut for: at it, an open mark
     * like a transport ring reads as clearly lit with its ring and its inner
     * figure still crisp. A dense drawing needs less again, since the keyboard
     * indicator's white keys are 1.24 px apart and the blur closes those gaps
     * long before it reaches this.
     */
    property real glowStrength: 0.7

    readonly property real level: Math.min(1, base + amp * TransportGlow.phase)

    property real base: (root.engaged || root.pressed) ? 1 : 0
    property real amp: (root.hovered && !root.engaged && !root.pressed) ? 0.85 : 0

    Behavior on base { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
    Behavior on amp { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }

    // The shared oscillator only ticks while a glyph has use for it, so an
    // untouched screen costs nothing. Counted off the amplitude rather than off
    // `hovered`, so the phase keeps running through the half-second fade-out.
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
        source: root.source
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
        spread: 0.1 + 0.45 * root.level * root.glowStrength
        opacity: root.level * root.glowStrength
        transparentBorder: true
        visible: opacity > 0
    }

    ColorOverlay {
        anchors.fill: iconSource
        source: iconSource
        // The theme's metal at rest, warming towards the halo's colour as
        // the glow rises.
        color: Qt.rgba(Theme.metal.r + (Theme.metalBright.r - Theme.metal.r) * root.level,
                       Theme.metal.g + (Theme.metalBright.g - Theme.metal.g) * root.level,
                       Theme.metal.b + (Theme.metalBright.b - Theme.metal.b) * root.level,
                       1)
    }
}
