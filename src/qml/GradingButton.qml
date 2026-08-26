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
import Qt5Compat.GraphicalEffects

import Orchestrion 1.0

// The grading master toggle, centred at the top of the score view: the
// graduation cap is lit when grading is on and dimmed when off — the same
// affordance the gesture-controller icons use for unavailable. Its dependent
// settings live in the Grading menu's settings dialog, out of the way.
Item {
    id: root

    width: 36
    height: 36

    property var model

    Image {
        id: gradingIcon
        anchors.fill: parent
        anchors.margins: 4
        source: "qrc:/icons/player/grading.svg"
        fillMode: Image.PreserveAspectFit
        mipmap: true
        visible: false
    }

    ColorOverlay {
        anchors.fill: gradingIcon
        source: gradingIcon
        color: Theme.accent
        // Dimmed while grading is off. Set here rather than on the root,
        // whose opacity carries the controls-row fade — the two multiply.
        opacity: root.model && root.model.gradingEnabled ? 1 : 0.25
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    MouseArea {
        id: gradingMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.model.gradingEnabled = !root.model.gradingEnabled
    }

    ToolTip {
        delay: 500
        visible: gradingMouseArea.containsMouse
        text: root.model && root.model.gradingEnabled ? qsTr("Grading (on)")
                                                      : qsTr("Grading (off)")
    }
}
