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
    readonly property color accent: "#2b7de9"
    readonly property color keyColor: "#ffffff"
    readonly property color keyBorder: "#b0b4ba"
    readonly property color keyText: "#2b2f36"
    readonly property color panelColor: Qt.rgba(1, 1, 1, 0.96)

    // Greyed context keys (neighbours + letter row)
    readonly property color greyKeyColor: "#e9ebee"
    readonly property color greyKeyText: "#9aa0a8"
    readonly property color greyKeyBorder: "#c7ccd2"

    // Finger + fingernail palette
    readonly property color skin: "#e9b08e"
    readonly property color skinLight: "#f4cbab"
    readonly property color skinDark: "#cf8f6b"
    readonly property color skinBorder: "#b07a57"
    readonly property color nailColor: "#f3d2cb"
    readonly property color nailHighlight: "#fceef0"
    readonly property color nailBorder: "#d9b1a4"

    function keyIsPressed(n) {
        return n !== 0 && (n === model.leftPressedKey || n === model.rightPressedKey)
    }

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

        // dimensions of the schematic keyboard
        readonly property int keyW: 40
        readonly property int keyH: 42
        readonly property int keyGap: 8
        readonly property int rowGap: 8
        readonly property real keyPitch: keyW + keyGap
        readonly property int letterRowY: keyH + rowGap
        readonly property real stagger: keyPitch / 2
        // number row: ` 1 2 3 4 5 6 7 8 9 0 -  (12 keys)
        readonly property int numberCount: 12
        readonly property real numberRowWidth: numberCount * keyPitch - keyGap
        readonly property int fingerTipRest: keyH
        readonly property int fingerTipPressed: keyH - 4
        readonly property int fingerLenLong: 78
        readonly property int fingerLenShort: 66
        readonly property int keyboardHeight: fingerTipRest + fingerLenLong + 6

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

            // A slice of keyboard: the number row framed by neighbouring keys and the
            // letter row below, greyed for context. Fingers reach up from below — past
            // the letter row — to hit the number keys, as the player sees them.
            Item {
                id: keyboard
                anchors.horizontalCenter: parent.horizontalCenter
                width: overlay.numberRowWidth
                height: overlay.keyboardHeight

                // ---- Letter row (greyed), drawn first so the fingers pass in front ----
                Row {
                    y: overlay.letterRowY
                    x: overlay.stagger
                    spacing: overlay.keyGap
                    Repeater {
                        model: ["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"]
                        delegate: Rectangle {
                            required property var modelData
                            width: overlay.keyW
                            height: overlay.keyH
                            radius: 6
                            color: root.greyKeyColor
                            border.color: root.greyKeyBorder
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.greyKeyText
                                font.pixelSize: 16
                            }
                        }
                    }
                }

                // ---- Number row: greyed neighbours + the active number keys ----
                Row {
                    y: 0
                    spacing: overlay.keyGap
                    Repeater {
                        model: [
                            { label: "`", n: -1 },
                            { label: "1", n: 1 }, { label: "2", n: 2 },
                            { label: "3", n: 3 }, { label: "4", n: 4 },
                            { label: "5", n: 5 }, { label: "6", n: 6 },
                            { label: "7", n: 7 }, { label: "8", n: 8 },
                            { label: "9", n: 9 }, { label: "0", n: 0 },
                            { label: "-", n: -1 }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            readonly property bool context: modelData.n < 0
                            readonly property bool pressed: root.keyIsPressed(modelData.n)
                            width: overlay.keyW
                            height: overlay.keyH
                            radius: 6
                            color: pressed ? root.accent
                                           : context ? root.greyKeyColor : root.keyColor
                            border.color: pressed ? root.accent
                                                  : context ? root.greyKeyBorder : root.keyBorder
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 90 } }
                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                color: pressed ? "white"
                                               : context ? root.greyKeyText : root.keyText
                                font.pixelSize: context ? 16 : 18
                                font.bold: !context
                            }
                        }
                    }
                }

                // ---- Fingers, drawn last (on top), reaching up to keys 2,3,8,9 ----
                Repeater {
                    model: [
                        { n: 2, long: true,  tilt: -3 }, // left middle
                        { n: 3, long: false, tilt: 3 },  // left index
                        { n: 8, long: false, tilt: -3 }, // right index
                        { n: 9, long: true,  tilt: 3 }   // right middle
                    ]
                    delegate: Item {
                        id: finger
                        required property var modelData
                        readonly property bool pressed: root.keyIsPressed(modelData.n)
                        width: 22
                        height: modelData.long ? overlay.fingerLenLong : overlay.fingerLenShort
                        // centre over the target number key (key index == digit for 2,3,8,9)
                        x: modelData.n * overlay.keyPitch + overlay.keyW / 2 - width / 2
                        // y is the finger tip (its top): at rest it waits below the number key;
                        // when pressed it rises to meet it.
                        y: pressed ? overlay.fingerTipPressed : overlay.fingerTipRest
                        rotation: modelData.tilt
                        transformOrigin: Item.Top

                        Behavior on y { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

                        // Finger body — horizontal gradient gives a cylindrical, rounded look
                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            border.color: root.skinBorder
                            border.width: 1
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: root.skinDark }
                                GradientStop { position: 0.35; color: root.skin }
                                GradientStop { position: 0.62; color: root.skinLight }
                                GradientStop { position: 1.0; color: root.skinDark }
                            }
                        }

                        // Knuckle crease (toward the hand, lower on the finger)
                        Rectangle {
                            width: parent.width * 0.66
                            height: 2
                            radius: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: parent.height * 0.55
                            color: root.skinBorder
                            opacity: 0.45
                        }

                        // Fingernail at the tip (top)
                        Rectangle {
                            width: parent.width * 0.56
                            height: parent.width * 0.72
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 5
                            radius: width * 0.45
                            border.color: root.nailBorder
                            border.width: 1
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: root.nailHighlight }
                                GradientStop { position: 1.0; color: root.nailColor }
                            }
                        }
                    }
                }
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
