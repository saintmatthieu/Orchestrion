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
import QtQuick.Layouts 1.15

// Which hand the machine plays for you, following your tempo. Opened from
// the top-row auto-play button and from the Auto-play menu.
Popup {
    id: root

    property var model

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 24

    background: Rectangle {
        color: "#F5241811"
        border.color: "#E5B84B"
        border.width: 2
        radius: 12
    }

    readonly property color ink: "#F0E5C8"
    readonly property color dimInk: "#C9B583"

    contentItem: ColumnLayout {
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Auto-play")
                color: root.ink
                font.pixelSize: 14
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "✕"
                color: root.dimInk
                font.pixelSize: 16

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        // `text` must carry the label even with a custom contentItem: the
        // style centres the indicator when it is empty.
        component HandButton: RadioButton {
            id: radio
            //! The staff the machine plays: −1 = none, 0 = right, 1 = left.
            property int staff
            checked: root.model && root.model.autoPlayedStaff === staff
            onClicked: root.model.autoPlayedStaff = staff
            contentItem: Text {
                text: radio.text
                color: root.ink
                font.pixelSize: 14
                leftPadding: radio.indicator.width + 8
                verticalAlignment: Text.AlignVCenter
            }
        }

        HandButton { text: qsTr("Off — I play both hands"); staff: -1 }
        HandButton { text: qsTr("Left hand"); staff: 1 }
        HandButton { text: qsTr("Right hand"); staff: 0 }
    }
}
