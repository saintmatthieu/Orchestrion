/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * Adapted from MuseScore's appshell QML (GPL-3.0-only,
 * Copyright (C) 2021 MuseScore BVBA and others).
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
import QtQuick.Window 2.15
import Orchestrion 1.0

// The in-window title bar (Windows and Linux): Orchestrion's menus and the
// system buttons; the empty strip between them drags the window (Main.qml's
// move area). Kept visible in full screen. Painted mahogany with cream text,
// like the macOS title bar (MacWindowChrome), so it blends into the wallpaper
// whatever the MuseScore UI theme.
Rectangle {
    id: root

    color: Theme.mahogany

    property int windowVisibility: Window.Windowed
    property alias appWindow: menu.appWindow

    signal showWindowMinimizedRequested()
    signal toggleWindowMaximizedRequested()
    signal closeWindowRequested()

    height: content.childrenRect.height

    RowLayout {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 8

        OrchestrionMenuBar {
            id: menu

            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.fillWidth: menu.truncated ? true : false
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: implicitHeight

            availableWidth: root.width - (content.spacing + systemButtons.width)
        }

        // Pushes the system buttons to the right edge.
        Item {
            Layout.fillWidth: !menu.truncated ? true : false
        }

        OrchestrionSystemButtons {
            id: systemButtons

            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.preferredWidth: width
            Layout.preferredHeight: height
            Layout.minimumWidth: width

            windowIsMiximized: root.windowVisibility === Window.Maximized

            onShowWindowMinimizedRequested: {
                root.showWindowMinimizedRequested()
            }

            onToggleWindowMaximizedRequested: {
                root.toggleWindowMaximizedRequested()
            }

            onCloseWindowRequested: {
                root.closeWindowRequested()
            }
        }
    }
}
