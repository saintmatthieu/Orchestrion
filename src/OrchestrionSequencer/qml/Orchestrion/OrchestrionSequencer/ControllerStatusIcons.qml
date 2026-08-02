/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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
import Qt5Compat.GraphicalEffects
import Orchestrion 1.0
import Orchestrion.OrchestrionSequencer 1.0

// Status icons for physical controllers. Today just the MIDI keyboard; a
// future gamepad icon would sit beside it in this row with the same
// connected/disconnected treatment. Controller selection itself lives in
// the Advanced ▸ Controllers menu.
Row {
    spacing: 8

    ControllerStatusModel {
        id: statusModel
        Component.onCompleted: statusModel.init()
    }

    Item {
        id: midiIcon
        width: 30
        height: 30
        opacity: statusModel.midiConnected ? 1 : 0.35

        Image {
            id: midiIconSource
            anchors.fill: parent
            source: "qrc:/icons/controllers/piano_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"
            sourceSize: Qt.size(width, height)
            visible: false
        }
        ColorOverlay {
            anchors.fill: midiIconSource
            source: midiIconSource
            color: Theme.accent
        }

        MouseArea {
            id: midiHoverArea
            anchors.fill: parent
            hoverEnabled: true
        }
        ToolTip {
            visible: midiHoverArea.containsMouse
            delay: 300
            // Below the icon, so the tooltip does not cover the menu bar.
            y: parent.height + 6
            text: statusModel.midiConnected ? qsTr("MIDI keyboard connected")
                                            : qsTr("MIDI keyboard disconnected")
        }
    }
}
