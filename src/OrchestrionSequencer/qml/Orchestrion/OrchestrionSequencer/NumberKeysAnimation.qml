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

// A slice of computer keyboard: the number row framed by neighbouring keys
// and the letter row below, greyed for context. Fingers reach up from below —
// past the letter row — to hit the number keys, as the player sees them.
// Drive it via leftPressedKey / rightPressedKey (0 = none); used by the
// NumberKeysHelp demo (following actual playback) and the welcome tour
// (looping canned pattern).
Item {
    id: root

    property int leftPressedKey: 0
    property int rightPressedKey: 0

    // Palette
    readonly property color accent: "#2b7de9"
    readonly property color keyColor: "#ffffff"
    readonly property color keyBorder: "#b0b4ba"
    readonly property color keyText: "#2b2f36"

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

    // Dimensions of the schematic keyboard
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

    implicitWidth: numberRowWidth
    implicitHeight: keyboardHeight

    function keyIsPressed(n) {
        return n !== 0 && (n === root.leftPressedKey || n === root.rightPressedKey)
    }

    // ---- Letter row (greyed), drawn first so the fingers pass in front ----
    Row {
        y: root.letterRowY
        x: root.stagger
        spacing: root.keyGap
        Repeater {
            model: ["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"]
            delegate: Rectangle {
                required property var modelData
                width: root.keyW
                height: root.keyH
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
        spacing: root.keyGap
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
                width: root.keyW
                height: root.keyH
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
            height: modelData.long ? root.fingerLenLong : root.fingerLenShort
            // centre over the target number key (key index == digit for 2,3,8,9)
            x: modelData.n * root.keyPitch + root.keyW / 2 - width / 2
            // y is the finger tip (its top): at rest it waits below the number key;
            // when pressed it rises to meet it.
            y: pressed ? root.fingerTipPressed : root.fingerTipRest
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
