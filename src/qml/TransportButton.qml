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

import Muse.Ui as MUI // qualified, like Main.qml's

import Orchestrion 1.0

/**
 * One playback control: a MetalGlyph that lights up while its action runs, so
 * play stays a triangle while playing and loop stays a loop while looping.
 */
Item {
    id: root

    /** File name of the glyph in qrc:/icons/player/, e.g. "play.svg". */
    property string icon

    /** The action this button stands for is currently running. */
    property bool engaged: false

    property string tooltip

    /**
     * Its place in keyboard navigation, and the name scripts reach it by
     * (testflow, demos): set its name, panel and order.
     */
    property alias navigation: navCtrl

    signal clicked()

    width: 36
    height: 36

    MetalGlyph {
        anchors.fill: parent
        source: "qrc:/icons/player/" + root.icon
        engaged: root.engaged
        hovered: mouseArea.containsMouse
        focused: navCtrl.highlight
        pressed: mouseArea.pressed
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    MUI.NavigationControl {
        id: navCtrl
        enabled: root.enabled // faded out too: Tab brings them back (Main.qml)

        accessible.role: MUI.MUAccessible.Button
        accessible.name: root.tooltip
        accessible.visualItem: root
        accessible.enabled: navCtrl.enabled

        onTriggered: root.clicked()
    }

    OrchestrionToolTip {
        visible: mouseArea.containsMouse && root.tooltip.length > 0
        text: root.tooltip
    }
}
