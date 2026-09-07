/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
 *
 * Adapted from MuseScore's interactive QML (GPL-3.0-only,
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
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents
import Muse.Interactive
import Orchestrion 1.0

// Orchestrion's take on MuseScore's StandardDialog (the question, info,
// warning and error boxes), registered for the same muse://interactive/standard
// URI. On Windows and Linux the dialog draws its own frame, like the main
// window: the system's title bar (Qt's Adwaita decoration on GNOME, the native
// one on Windows) knows nothing of the mahogany palette and shows the title in
// stark white on grey. Here the title bar is part of the card, with the title
// in cream and a close button, and it drags the window. On macOS the native
// title bar is kept, as for the main window.
StyledDialogView {
    id: root

    property alias type: mainPanel.type

    property alias dialogTitle: root.title
    property alias contentTitle: mainPanel.title
    property alias text: mainPanel.text
    property alias textFormat: mainPanel.textFormat
    property string detailedText: ""

    property alias withIcon: mainPanel.withIcon
    property alias iconCode: mainPanel.iconCode

    property alias withDontShowAgainCheckBox: mainPanel.withDontShowAgainCheckBox

    property var buttons
    property var customButtons
    property alias defaultButtonId: mainPanel.defaultButtonId

    readonly property bool isMac: Qt.platform.os === "osx"
    readonly property int bodyMargins: 16

    frameless: !root.isMac

    QtObject {
        id: toggleDetailsButton

        property int buttonId: 999
        property string text: detailsLoader.active ? qsTrc("global", "Hide details") : qsTrc("global", "Show details")
        property int role: ButtonBoxModel.CustomRole
        property bool isAccent: false
        property bool isLeftSide: true
    }

    contentWidth: mainPanel.implicitWidth + 2 * root.bodyMargins
    contentHeight: layout.implicitHeight

    // The title bar spans the whole card; the body has its own margins.
    margins: 0

    onDetailedTextChanged: {
        if (root.detailedText.length <= 0) {
            return
        }

        var tmp = []
        tmp.push(toggleDetailsButton)

        for (var i = 0; i < root.customButtons.length; ++i) {
            tmp.push(root.customButtons[i])
        }

        root.customButtons = tmp
    }

    onNavigationActivateRequested: {
        mainPanel.focusOnFirst()
    }

    onAccessibilityActivateRequested: {
        mainPanel.readInfo()
    }

    Column {
        id: layout

        width: root.contentWidth

        // The in-card title bar (Windows and Linux only, see above).
        Item {
            id: titleBar

            visible: root.frameless
            width: parent.width
            // Room for the card's 1px border above and below the button.
            height: closeButton.height + 2

            MouseArea {
                anchors.fill: parent
                onPressed: titleBar.Window.window.startSystemMove()
            }

            StyledTextLabel {
                anchors.left: parent.left
                anchors.leftMargin: root.bodyMargins
                anchors.right: closeButton.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter

                text: root.title
                font: ui.theme.bodyBoldFont
                color: Theme.accent
                horizontalAlignment: Text.AlignLeft
            }

            FlatButton {
                id: closeButton

                anchors.right: parent.right
                anchors.rightMargin: 1
                anchors.verticalCenter: parent.verticalCenter

                icon: IconCode.APP_CLOSE
                iconColor: Theme.accent
                transparent: true
                drawFocusBorderInsideRect: true
                backgroundItem: TitleBarButtonBackground {
                    mouseArea: closeButton.mouseArea
                }

                onClicked: {
                    root.close()
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: ui.theme.strokeColor
            }
        }

        Column {
            id: content

            x: root.bodyMargins
            width: parent.width - 2 * root.bodyMargins
            topPadding: root.bodyMargins
            bottomPadding: root.bodyMargins
            spacing: 16

            StandardDialogPanel {
                id: mainPanel

                navigation.section: root.navigationSection
                navigation.order: 1

                buttons: root.buttons
                customButtons: root.customButtons

                onClicked: function(buttonId, showAgain) {
                    if (buttonId === toggleDetailsButton.buttonId) {
                        detailsLoader.active = !detailsLoader.active
                        return
                    }

                    root.ret = { "errcode": 0, "value": { "buttonId": buttonId, "showAgain": showAgain }}
                    root.hide()
                }
            }

            Loader {
                id: detailsLoader

                width: parent.width
                height: visible ? implicitHeight : 0

                active: false
                visible: active

                sourceComponent: ErrorDetailsView {
                    detailedText: root.detailedText

                    navigationSection: root.navigationSection
                    navigationOrder: 2
                }
            }
        }
    }
}
