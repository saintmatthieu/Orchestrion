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
pragma Singleton
import QtQuick 2.15

/**
 * The breath the transport buttons glow to: one oscillator shared by the whole
 * row, so they swell together and a button that starts glowing joins the cycle
 * already in progress rather than restarting it.
 *
 * TransportButton never animates its glow directly — it eases a target that is
 * multiplied by this phase — so nothing here may restart, or the level would
 * jump. It is therefore paused rather than stopped when no button needs it,
 * which holds the phase where it stood; `users` counts the buttons that do.
 */
QtObject {
    id: root

    // Swells and recedes over three seconds, the rate of a calm breath.
    property real phase: 0

    // Buttons currently fading in, breathing, or fading out.
    property int users: 0

    property SequentialAnimation pulse: SequentialAnimation {
        running: true
        paused: root.users === 0
        loops: Animation.Infinite

        NumberAnimation {
            target: root
            property: "phase"
            to: 1
            duration: 1500
            easing.type: Easing.InOutSine
        }

        NumberAnimation {
            target: root
            property: "phase"
            to: 0
            duration: 1500
            easing.type: Easing.InOutSine
        }
    }
}
