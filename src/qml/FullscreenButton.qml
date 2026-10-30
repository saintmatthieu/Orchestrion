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
import QtQuick.Window 2.15

import Muse.Ui as MUI // qualified, like Main.qml's

import Orchestrion 1.0

TransportButton {
    id: root

    /** The navigation section the button's panel belongs to. */
    property MUI.NavigationSection navigationSection

    icon: "fullscreen.svg"
    tooltip: qsTr("Full screen")
    navigation.name: "FullScreen"
    navigation.panel: navPanel
    navigation.order: 1

    MUI.NavigationPanel {
        id: navPanel
        name: "View"
        section: root.navigationSection
        order: 2
        enabled: root.enabled // faded out too: Tab brings them back (Main.qml)
        accessible.name: qsTr("View")
    }
    engaged: Window.window && Window.window.visibility === Window.FullScreen

    onClicked: {
        const w = Window.window
        if (w.visibility === Window.FullScreen) {
            w.showMaximized()
        } else {
            w.showFullScreen()
        }
    }
}
