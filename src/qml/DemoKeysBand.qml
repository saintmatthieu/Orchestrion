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
 * What demos show of the computer keyboard (DemoKeys), a row each under the
 * score: a caption, then the number keys 1 to 0, the left hand's half and the
 * right hand's, going down as they are pressed. Each row opens as it comes and
 * closes as it goes, and bandHeight says how much room they take. Takes no
 * input.
 */
Item {
    id: root

    readonly property int keySize: 56
    readonly property int keyGap: 8
    // Between the two hands' halves
    readonly property int handGap: 32
    // How far a key sinks when pressed, showing as much of its base at rest
    readonly property int travel: 5
    // Under each row
    readonly property int rowGap: 12

    property real captionOpen: DemoKeys.caption.length > 0 ? 1 : 0
    Behavior on captionOpen {
        NumberAnimation { duration: 350; easing.type: Easing.InOutCubic }
    }
    property real keysOpen: DemoKeys.shown ? 1 : 0
    Behavior on keysOpen {
        NumberAnimation { duration: 350; easing.type: Easing.InOutCubic }
    }

    readonly property real bandHeight: (captionPill.height + rowGap) * captionOpen
                                       + (keys.height + rowGap) * keysOpen

    enabled: false

    Rectangle {
        id: captionPill
        anchors.horizontalCenter: parent.horizontalCenter
        width: caption.implicitWidth + 48
        height: caption.implicitHeight + 20
        radius: height / 2
        color: Theme.overlay
        border.color: Theme.metal
        border.width: 1

        opacity: root.captionOpen
        visible: opacity > 0

        Text {
            id: caption
            anchors.centerIn: parent
            // Kept while fading out
            property string shown: ""
            text: shown
            color: Theme.metalBright
            font.pixelSize: 28

            Connections {
                target: DemoKeys
                function onCaptionChanged() {
                    if (DemoKeys.caption.length > 0) {
                        caption.shown = DemoKeys.caption
                    }
                }
            }
        }
    }

    Row {
        id: keys
        anchors.horizontalCenter: parent.horizontalCenter
        y: (captionPill.height + root.rowGap) * root.captionOpen
        spacing: root.handGap

        opacity: root.keysOpen
        visible: opacity > 0

        Repeater {
            // The left hand's keys, then the right hand's
            model: [["1", "2", "3", "4", "5"], ["6", "7", "8", "9", "0"]]

            Row {
                readonly property int half: index
                spacing: root.keyGap

                Repeater {
                    model: modelData

                    Item {
                        readonly property bool down:
                            (DemoKeys.pressedKeys >> (parent.half * 5 + index)) & 1
                        width: root.keySize
                        height: root.keySize + root.travel

                        // The base the key sinks onto
                        Rectangle {
                            width: root.keySize
                            height: root.keySize
                            y: root.travel
                            radius: 9
                            color: Qt.darker(Theme.backdrop, 1.6)
                            border.color: Theme.metal
                            border.width: 1
                        }

                        Rectangle {
                            width: root.keySize
                            height: root.keySize
                            y: down ? root.travel : 0
                            radius: 9
                            color: down ? Theme.metalBright : Theme.backdrop
                            border.color: down ? Theme.metalBright : Theme.metal
                            border.width: 1.5
                            Behavior on y { NumberAnimation { duration: 40 } }

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: down ? Theme.backdrop : Theme.metalBright
                                font.pixelSize: 24
                            }
                        }
                    }
                }
            }
        }
    }
}
