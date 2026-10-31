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
 * What demos show of the computer keyboard (DemoKeys): the number keys 1 to 0,
 * the left hand's half and the right hand's, going down as they are pressed,
 * under a caption. Takes no input.
 */
Item {
    id: root

    readonly property int keySize: 56
    readonly property int keyGap: 8
    // Between the two hands' halves
    readonly property int handGap: 32
    // How far a key sinks when pressed, showing as much of its base at rest
    readonly property int travel: 5

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

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        // Between the score and the transport row
        y: parent.height * 0.71
        spacing: 20

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: caption.implicitWidth + 48
            height: caption.implicitHeight + 20
            radius: height / 2
            color: Theme.overlay
            border.color: Theme.metal
            border.width: 1

            opacity: DemoKeys.caption.length > 0 ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 300 } }

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
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.handGap

            opacity: DemoKeys.shown ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 300 } }

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
}
