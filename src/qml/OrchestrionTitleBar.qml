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
import Muse.Ui
import Muse.UiComponents

// The in-window title bar (Windows and Linux): Orchestrion's menus, the
// window title and the system buttons. Kept visible in full screen.
Rectangle {
    id: root

    color: ui.theme.backgroundPrimaryColor

    property alias title: titleTextmetrics.text
    property rect titleMoveAreaRect: Qt.rect(titleMoveArea.x, titleMoveArea.y, titleMoveArea.width, titleMoveArea.height)
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

            availableWidth: root.width - (content.spacing + titleLabel.minDistanceFromMenu + titleTextmetrics.width + content.spacing + systemButtons.width)
        }

        StyledTextLabel {
            id: titleLabel

            readonly property int minDistanceFromMenu: 24

            Layout.fillWidth: !menu.truncated ? true : false
            Layout.fillHeight: true
            Layout.minimumWidth: titleTextmetrics.advanceWidth

            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            leftPadding: {
                var parentCenterX = parent.width / 2
                var expectedTextCenterX = parentCenterX - titleTextmetrics.width / 2
                return Math.max(expectedTextCenterX - x, minDistanceFromMenu)
            }

            text: titleTextmetrics.elidedText
            textFormat: Text.RichText
            font: ui.theme.bodyFont

            TextMetrics {
                id: titleTextmetrics
                text: qsTrc("appshell", "Orchestrion")
                font: titleLabel.font
                elide: Qt.ElideRight
                elideWidth: titleLabel.width
            }
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

    Item {
        id: titleMoveArea

        x: titleLabel.x
        y: titleLabel.y

        width: titleLabel.visible ? titleLabel.width : 0
        height: titleLabel.visible ? titleLabel.height : 0
    }
}
