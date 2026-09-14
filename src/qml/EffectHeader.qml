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
import Muse.Ui
import Muse.UiComponents
import Orchestrion 1.0
import Orchestrion.OrchestrionSynthesis 1.0

// The header of an effect's window: the effect's name and its bypass button.
// Transparent, so it sits on whatever the window shows below. The effect is
// named either by id (built-in effects) or by the VST instance shown.
Item {
    id: root

    property alias effectId: header.effectId
    property alias vstInstanceId: header.vstInstanceId

    readonly property color gold: "#D4A858"

    implicitHeight: 36
    height: implicitHeight

    EffectHeaderModel {
        id: header
    }

    StyledTextLabel {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: bypassButton.left
        anchors.rightMargin: 12
        horizontalAlignment: Text.AlignLeft
        text: header.title
        font: ui.theme.bodyBoldFont
        color: Theme.accent
        opacity: header.active ? 1 : 0.6
    }

    // A power-style toggle: gold when the effect is in, filled gold when it
    // is bypassed (the switch is "on"), a cream tint on hover as on the title
    // bar's buttons.
    Rectangle {
        id: bypassButton

        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        width: 26
        height: 26
        radius: width / 2
        color: header.active ? "transparent" : root.gold
        border.color: root.gold
        border.width: 1

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: Theme.accent
            opacity: bypassArea.pressed ? 0.3 : bypassArea.containsMouse ? 0.15 : 0
        }

        StyledIconLabel {
            anchors.centerIn: parent
            iconCode: IconCode.BYPASS
            font.pixelSize: 16
            color: header.active ? root.gold : Theme.mahogany
        }

        MouseArea {
            id: bypassArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: header.toggleActive()
            onContainsMouseChanged: {
                if (containsMouse) {
                    ui.tooltip.show(bypassButton, header.active ? qsTrc("effects", "Bypass") : qsTrc("effects", "Bypassed: click to re-enable"))
                } else {
                    ui.tooltip.hide(bypassButton)
                }
            }
        }
    }
}
