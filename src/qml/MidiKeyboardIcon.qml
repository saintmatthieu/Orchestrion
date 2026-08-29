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
import Qt5Compat.GraphicalEffects
import Orchestrion 1.0
import Orchestrion.OrchestrionShell 1.0

// The MIDI keyboard indicator, top-left of the score view: a piano icon, dimmed
// while no MIDI keyboard is connected. Hovering it shows a panel underneath with
// the connection status and, on the icon's upper-right corner, a cross that
// hides the icon (the View menu's "MIDI keyboard icon" brings it back).
Item {
    id: root

    property int iconSize: 40

    //! Whether the user chose to see the icon at all.
    readonly property bool shown: model.iconVisible
    //! Whether the pointer is on the icon, its cross or its panel. A short
    //! grace period bridges the gaps between them, and the parent keeps the
    //! controls overlay up while this is true.
    property bool hovered: false

    // The panel is a slip of the score's own parchment.
    readonly property color panelColor: Theme.accent
    readonly property color panelBorder: Qt.rgba(0.14, 0.09, 0.07, 0.35)
    readonly property color panelText: "#241811"
    //! The icon dims while no keyboard is connected; the cross follows it.
    readonly property real dimOpacity: model.connected ? 1 : 0.25

    readonly property bool rawHovered: iconHover.hovered
                                       || crossArea.containsMouse
                                       || panelHover.hovered

    implicitWidth: iconSize
    implicitHeight: iconSize

    onRawHoveredChanged: {
        if (rawHovered) {
            unhoverTimer.stop()
            hovered = true
        } else {
            unhoverTimer.restart()
        }
    }

    Timer {
        id: unhoverTimer
        interval: 150
        onTriggered: root.hovered = false
    }

    MidiKeyboardIconModel {
        id: model
        Component.onCompleted: model.load()
    }

    // ---- The icon ---------------------------------------------------------
    Image {
        id: iconSource
        anchors.fill: parent
        source: model.iconSource
        sourceSize.width: root.iconSize
        sourceSize.height: root.iconSize
        visible: false
    }
    ColorOverlay {
        anchors.fill: iconSource
        source: iconSource
        color: Theme.accent
        opacity: root.dimOpacity
    }
    HoverHandler {
        id: iconHover
    }

    // ---- Dismiss cross, on the upper-right corner -------------------------
    Rectangle {
        id: cross
        visible: root.hovered
        width: 18
        height: 18
        radius: 9
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: -6
        anchors.rightMargin: -6
        // Greyed out like the icon (still clickable); full strength under
        // the pointer.
        opacity: crossArea.containsMouse ? 1 : root.dimOpacity
        color: crossArea.containsMouse ? "#ffffff" : Theme.accent
        border.color: root.panelText
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "×"
            color: root.panelText
            font.pixelSize: 14
            font.bold: true
        }
        MouseArea {
            id: crossArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: model.hide()
        }
    }

    // ---- Status panel, underneath -----------------------------------------
    Rectangle {
        id: panel
        visible: root.hovered
        anchors.top: parent.bottom
        anchors.topMargin: 6
        anchors.left: parent.left
        radius: 8
        color: root.panelColor
        border.color: root.panelBorder
        border.width: 1
        width: panelColumn.implicitWidth + 24
        height: panelColumn.implicitHeight + 16

        HoverHandler {
            id: panelHover
        }

        Column {
            id: panelColumn
            anchors.centerIn: parent
            spacing: 4

            Text {
                text: model.connected ? qsTr("MIDI keyboard: connected")
                                      : qsTr("MIDI keyboard: disconnected")
                color: root.panelText
                font.pixelSize: 14
                font.bold: true
            }
            Text {
                visible: !model.connected
                text: qsTr("Use a MIDI keyboard to get better control over nuances")
                color: root.panelText
                font.pixelSize: 13
            }
        }
    }
}
