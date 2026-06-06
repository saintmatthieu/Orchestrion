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

// Transient "this score must be attributed" notification.
// Instantiate with `anchors.fill: parent`; the toast places itself at the
// bottom-centre of that area and fades itself away after a few seconds.
Item {
    id: root

    ScoreAttributionModel {
        id: model

        onAttributionRequired: {
            card.opacity = 1
            dismissTimer.restart()
        }

        onAttributionChanged: {
            // A score with no attribution was opened: dismiss any toast still
            // showing for the previous score.
            if (model.author.length === 0) {
                dismissTimer.stop()
                card.opacity = 0
            }
        }
    }

    Component.onCompleted: model.load()

    Timer {
        id: dismissTimer
        interval: 7000
        repeat: false
        onTriggered: card.opacity = 0
    }

    Rectangle {
        id: card

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24

        width: layout.implicitWidth + 32
        height: layout.implicitHeight + 24
        radius: 8
        color: "#2B2B2B"
        border.color: Theme.accent
        border.width: 1

        opacity: 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 300 } }

        // Pause auto-dismiss while the user is hovering, so the source link
        // stays clickable.
        HoverHandler {
            onHoveredChanged: {
                if (hovered)
                    dismissTimer.stop()
                else if (card.opacity > 0)
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
                    color: Theme.accent
                    font.pixelSize: 14
                    font.bold: true
                }

                Text {
                    visible: model.license.length > 0
                    text: qsTr("Licensed under %1").arg(model.license)
                    color: "#CCCCCC"
                    font.pixelSize: 12
                }
            }

            Text {
                visible: model.url.length > 0
                text: qsTr("View source")
                color: Theme.accent
                font.pixelSize: 12
                font.underline: true

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: model.openSourceUrl()
                }
            }
        }
    }
}
