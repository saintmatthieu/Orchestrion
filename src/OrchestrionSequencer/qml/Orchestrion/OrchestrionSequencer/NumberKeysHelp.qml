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
import Orchestrion.OrchestrionSequencer 1.0

// Beginner help shown when no MIDI controller is connected: a persistent
// "Use your number keys to play." tooltip with a "Show me!" button that starts
// playback and animates a schematic number-key row, pressing 2/3 (left hand)
// and 8/9 (right hand) in time with the music.
Item {
    id: root

    // Palette
    readonly property color keyBorder: "#b0b4ba"
    readonly property color keyText: "#2b2f36"
    readonly property color panelColor: Qt.rgba(1, 1, 1, 0.96)

    NumberKeysHelpModel {
        id: model
        Component.onCompleted: model.init()
    }

    // ---- Persistent tooltip ---------------------------------------------
    Rectangle {
        id: tooltip
        visible: model.tooltipVisible
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        radius: 8
        color: root.panelColor
        border.color: root.keyBorder
        border.width: 1
        width: tooltipRow.implicitWidth + 28
        height: tooltipRow.implicitHeight + 20

        Row {
            id: tooltipRow
            anchors.centerIn: parent
            spacing: 12

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Use your number keys to play.")
                color: root.keyText
                font.pixelSize: 15
            }

            Button {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Show me!")
                onClicked: model.showMe()
            }

            // Dismiss
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 22
                height: 22
                radius: 11
                color: closeArea.containsMouse ? "#e7e9ec" : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "×" // ×
                    color: root.keyText
                    font.pixelSize: 16
                }
                MouseArea {
                    id: closeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: model.dismiss()
                }
            }
        }
    }

    // ---- Keyboard animation overlay -------------------------------------
    Rectangle {
        id: overlay
        visible: model.demoActive
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        radius: 12
        color: root.panelColor
        border.color: root.keyBorder
        border.width: 1
        width: overlayColumn.implicitWidth + 48
        height: overlayColumn.implicitHeight + 36

        Column {
            id: overlayColumn
            anchors.centerIn: parent
            spacing: 14

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Use your number keys to play.")
                color: root.keyText
                font.pixelSize: 16
                font.bold: true
            }

            NumberKeysAnimation {
                anchors.horizontalCenter: parent.horizontalCenter
                leftPressedKey: model.leftPressedKey
                rightPressedKey: model.rightPressedKey
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Left hand: keys 1 to 5   •   Right hand: keys 6 to 0")
                color: root.keyText
                font.pixelSize: 13
            }
        }

        // Close the demo
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 8
            anchors.rightMargin: 8
            width: 24
            height: 24
            radius: 12
            color: overlayCloseArea.containsMouse ? "#e7e9ec" : "transparent"
            Text {
                anchors.centerIn: parent
                text: "×"
                color: root.keyText
                font.pixelSize: 17
            }
            MouseArea {
                id: overlayCloseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: model.stop()
            }
        }
    }
}
