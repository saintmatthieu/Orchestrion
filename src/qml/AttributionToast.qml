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
import QtQuick.Layouts 1.15

import Orchestrion 1.0
import Orchestrion.OrchestrionShell 1.0

// The credit a score must carry (e.g. a Creative-Commons transcription),
// shown for a few seconds when it is opened, centred, its top at `cardTop`: in
// the row of the cards of the scores before and after (Main.qml), taking no
// room, so that the score stays where it is from one score to the next.
// Stays while hovered, so that its link can be clicked.
Item {
    id: root

    /** Where its top is, from its parent's. */
    property real cardTop: 0
    /** Its card's height, open or not. */
    readonly property real cardHeight: card.height

    property real open: 0
    Behavior on open {
        NumberAnimation { duration: 350; easing.type: Easing.InOutCubic }
    }

    ScoreAttributionModel {
        id: model

        onAttributionRequired: {
            root.open = 1
            dismissTimer.restart()
        }

        onAttributionChanged: {
            // A score with no attribution was opened: dismiss any credit
            // still showing for the previous score.
            if (model.author.length === 0) {
                dismissTimer.stop()
                root.open = 0
            }
        }
    }

    Component.onCompleted: model.load()

    Timer {
        id: dismissTimer
        interval: 7000
        repeat: false
        onTriggered: root.open = 0
    }

    // On the parchment: the backdrop's colour, framed in metal, as the other
    // cards under the score.
    Rectangle {
        id: card

        anchors.horizontalCenter: parent.horizontalCenter
        y: root.cardTop

        width: layout.implicitWidth + 32
        height: layout.implicitHeight + 24
        radius: 8
        color: Theme.backdrop
        border.color: Theme.metal
        border.width: 1

        opacity: root.open
        visible: opacity > 0

        // Pause auto-dismiss while the user is hovering, so the source link
        // stays clickable.
        HoverHandler {
            onHoveredChanged: {
                if (hovered)
                    dismissTimer.stop()
                else if (root.open > 0)
                    dismissTimer.restart()
            }
        }

        RowLayout {
            id: layout
            anchors.centerIn: parent
            spacing: 16

            ColumnLayout {
                spacing: 2

                Text {
                    text: qsTr("Sheet music by %1").arg(model.author)
                    color: Theme.metalBright
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }

                Text {
                    visible: model.license.length > 0
                    text: qsTr("Licensed under %1").arg(model.license)
                    color: Theme.inkMuted
                    font.pixelSize: 12
                }
            }

            Text {
                visible: model.url.length > 0
                text: qsTr("View source")
                color: sourceMouse.containsMouse ? Theme.metalBright : Theme.metal
                font.pixelSize: 12
                font.underline: true

                MouseArea {
                    id: sourceMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: model.openSourceUrl()
                }
            }
        }
    }
}
