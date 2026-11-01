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
 * What says a tutorial is running, over everything else in the window. Takes no
 * input.
 */
Item {
    id: root

    enabled: false

    // While a tutorial runs in the user's session (Tutorials)
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        // Below the menu bar, when the window has one
        y: 44
        width: tutorialBadge.implicitWidth + 28
        height: tutorialBadge.implicitHeight + 12
        radius: height / 2
        color: Theme.overlay
        border.color: Theme.metal
        border.width: 1
        opacity: Tutorials.running ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 300 } }

        Text {
            id: tutorialBadge
            anchors.centerIn: parent
            text: qsTr("Tutorial · Esc or Stop to end it")
            color: Theme.inkMuted
            font.pixelSize: 13
        }
    }
}
