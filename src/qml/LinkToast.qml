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

// What became of a link that asked for a score: it stays while the score is
// being fetched, and otherwise goes by itself, or when clicked.
Rectangle {
    id: toast

    property string message: ""
    property bool busy: false
    signal dismissed()

    width: Math.min(label.implicitWidth + 40, (parent ? parent.width : 480) - 40)
    height: label.implicitHeight + 24
    radius: 8
    color: Theme.backdrop
    border.color: Theme.metal
    border.width: 1
    visible: opacity > 0
    opacity: message !== "" ? 1 : 0
    Behavior on opacity {
        NumberAnimation { duration: 250 }
    }

    Text {
        id: label
        anchors.fill: parent
        anchors.margins: 12
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        text: toast.message
        color: Theme.metalBright
        font.pixelSize: 14
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        anchors.fill: parent
        enabled: !toast.busy
        onClicked: toast.dismissed()
    }

    Timer {
        interval: 7000
        running: toast.message !== "" && !toast.busy
        onTriggered: toast.dismissed()
    }
}
