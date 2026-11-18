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
import QtQuick
import Orchestrion.OrchestrionSynthesis 1.0

// After an audible underrun with a latency the user chose (not automatic):
// the way to the latency dialog. Stays until closed: the ×, till the next
// glitch, or "Don't show again", for good. Styled as the "New here?" card; Main.qml
// gives it a row under the score, and its opacity.
Rectangle {
    id: toast

    readonly property bool shown: hint.shown

    GlitchHintModel {
        id: hint
    }

    width: row.implicitWidth + 28
    height: row.implicitHeight + 20
    radius: 8
    color: Theme.backdrop
    border.color: Theme.metal
    border.width: 1
    visible: opacity > 0

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 12

        // One sentence for the translators, its link where they put it
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Audio glitch? Adjust your <a href=\"latency\">latency settings</a>")
            textFormat: Text.StyledText
            color: Theme.metalBright
            linkColor: Theme.metalBright
            font.pixelSize: 14
            onLinkActivated: hint.openSettings()

            HoverHandler {
                cursorShape: parent.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: neverLabel.implicitWidth + 24
            height: neverLabel.implicitHeight + 10
            radius: height / 2
            color: "transparent"
            border.color: neverMouse.containsMouse ? Theme.metalBright : Theme.metal

            Text {
                id: neverLabel
                anchors.centerIn: parent
                text: qsTr("Don't show again")
                color: neverMouse.containsMouse ? Theme.metalBright : Theme.metal
                font.pixelSize: 13
            }

            MouseArea {
                id: neverMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: hint.closeForever()
            }
        }

        // Close
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 22
            height: 22
            radius: 11
            color: closeMouse.containsMouse ? Theme.overlay : "transparent"

            Text {
                anchors.centerIn: parent
                text: "×"
                color: Theme.inkMuted
                font.pixelSize: 16
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: hint.close()
            }
        }
    }
}
