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

import Muse.Ui 1.0

Rectangle {

    readonly property int leftPadding: 5

    color: appIconMouseArea.containsMouse ? ui.theme.backgroundSecondaryColor : ui.theme.backgroundPrimaryColor
    Image {
        id: appIcon
        source: "qrc:/icons/music-box.ico"
        anchors.fill: parent
        anchors.leftMargin: leftPadding + 5
        anchors.topMargin: 5
        anchors.rightMargin: 5
        anchors.bottomMargin: 5
        fillMode: Image.PreserveAspectFit
    }

    MouseArea {
        id: appIconMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: toolTip.visible = !toolTip.visible
    }

    ToolTip {
        id: toolTip
        x: 0
        y: titleBar.height + leftPadding
        timeout: 10000
        background: Rectangle {
            color: appTitleBar.color
            radius: 2
        }
        contentItem: Text {
            text: "<a href='https://www.flaticon.com/free-icons/music-box' title='music box icons'>Music box icons created by iconixar - Flaticon</a>"
            font.pointSize: 10
            verticalAlignment: Text.AlignTop
            textFormat: Text.RichText
            onLinkActivated: function(link) {
                Qt.openUrlExternally(link);
            }
        }
    }
}