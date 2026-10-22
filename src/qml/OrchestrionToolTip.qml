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

// Orchestrion's tooltip: a small popup-coloured slip edged with metal that
// fades in after a second of hovering and fades out when the mouse leaves.
// Declare it as a child of the hovered item and bind `visible` to its hover
// state (the attached ToolTip has the platform style, which clashes).
ToolTip {
    id: root

    readonly property int fadeDuration: 150
    // The text shown: the last non-empty one, so that a tooltip whose text is
    // cleared as it hides fades out with it rather than as an empty slip.
    property string shownText

    Component.onCompleted: shownText = text
    onTextChanged: {
        if (text.length > 0)
            shownText = text
    }

    delay: 1000
    padding: 0
    topPadding: 5
    bottomPadding: 6
    leftPadding: 10
    rightPadding: 10

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: root.fadeDuration
            easing.type: Easing.OutQuad
        }
    }
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: root.fadeDuration
            easing.type: Easing.InQuad
        }
    }

    contentItem: Text {
        text: root.shownText
        color: Theme.metalBright
        font.pixelSize: 12
        wrapMode: Text.Wrap
    }

    background: Rectangle {
        color: Theme.popup
        border.color: Theme.metal
        border.width: 1
        radius: 4
    }
}
