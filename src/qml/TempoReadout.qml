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
import QtQuick.Controls 2.15

import Orchestrion 1.0
import Orchestrion.OrchestrionShell 1.0

/**
 * The tempo where the player is, "♩ = 96", in the transport row. Scrolled or
 * dragged up and down, or double-clicked and typed into, it sets the tempo
 * the whole score is played at, its tempo changes kept in proportion. While
 * it is off the score's, the ratio shows beside it; clicking that goes back.
 *
 * A scroll or a drag is one edit of the score, made once it settles, not one
 * per step.
 */
Item {
    id: root

    implicitWidth: row.implicitWidth
    implicitHeight: 36
    visible: tempoModel.available

    TempoModel {
        id: tempoModel
    }

    Component.onCompleted: tempoModel.load()

    /** The tempo being set, not made an edit yet; -1 when none. */
    property int pending: -1
    readonly property int shown: pending >= 0 ? pending : tempoModel.bpm

    function nudge(steps) {
        pending = Math.max(tempoModel.minBpm,
                           Math.min(tempoModel.maxBpm, shown + steps))
        commitTimer.restart()
    }

    function commit() {
        commitTimer.stop()
        if (pending >= 0 && pending !== tempoModel.bpm)
            tempoModel.setBpm(pending)
        pending = -1
    }

    Timer {
        id: commitTimer
        interval: 500
        onTriggered: root.commit()
    }

    FontLoader {
        id: titleFont
        source: "qrc:/fonts/CinzelDecorative/CinzelDecorative-Regular.ttf"
    }

    Row {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6

        Text {
            id: readout
            anchors.verticalCenter: parent.verticalCenter
            visible: !editor.visible
            text: "\u2669 = " + root.shown
            color: mouseArea.containsMouse || mouseArea.pressed
                   ? Theme.metalBright : Theme.metal
            font.family: titleFont.name
            font.pixelSize: 18
            renderType: Text.NativeRendering

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                anchors.margins: -6
                hoverEnabled: true
                cursorShape: Qt.SizeVerCursor

                property real lastY: 0
                property real travel: 0

                onPressed: (mouse) => { lastY = mouse.y; travel = 0 }
                onPositionChanged: (mouse) => {
                    if (!pressed)
                        return
                    // A beat per minute every few pixels, upwards faster.
                    travel += lastY - mouse.y
                    lastY = mouse.y
                    const steps = Math.trunc(travel / 3)
                    if (steps !== 0) {
                        travel -= steps * 3
                        root.nudge(steps)
                    }
                }
                onReleased: root.commit()
                onWheel: (wheel) => {
                    const steps = Math.round(wheel.angleDelta.y / 120)
                    if (steps !== 0)
                        root.nudge(steps)
                }
                onDoubleClicked: {
                    root.commit()
                    editor.text = tempoModel.bpm
                    editor.visible = true
                    editor.forceActiveFocus()
                    editor.selectAll()
                }
            }

            ToolTip {
                delay: 500
                visible: mouseArea.containsMouse && !mouseArea.pressed
                text: qsTr("Tempo: scroll or drag to change it, double-click to type it")
            }
        }

        TextField {
            id: editor
            anchors.verticalCenter: parent.verticalCenter
            visible: false
            width: 64
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 16
            validator: IntValidator {
                bottom: tempoModel.minBpm
                top: tempoModel.maxBpm
            }
            onAccepted: {
                tempoModel.setBpm(parseInt(text))
                visible = false
            }
            Keys.onEscapePressed: visible = false
            onActiveFocusChanged: {
                if (!activeFocus)
                    visible = false
            }
        }

        Text {
            id: ratio
            anchors.verticalCenter: parent.verticalCenter
            // Its room is kept while hidden, so the transport row doesn't
            // shift whenever the tempo goes back to the score's.
            readonly property bool off: Math.abs(tempoModel.multiplier - 1) > 0.0005
            opacity: off ? 1 : 0
            text: "\u00d7" + (off ? tempoModel.multiplier : 1).toFixed(2)
            color: ratioArea.containsMouse ? Theme.metalBright : Theme.inkMuted
            font.pixelSize: 12
            renderType: Text.NativeRendering

            MouseArea {
                id: ratioArea
                anchors.fill: parent
                anchors.margins: -4
                enabled: ratio.off
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.pending = -1
                    commitTimer.stop()
                    tempoModel.reset()
                }
            }

            ToolTip {
                delay: 500
                visible: ratioArea.containsMouse
                text: qsTr("Back to the score's tempo")
            }
        }
    }
}
