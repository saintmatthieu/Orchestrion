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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import MuseScore.Ui 1.0
import MuseScore.UiComponents 1.0
import MuseScore.NotationScene 1.0
import MuseScore.AppShell 1.0
import MuseScore.Shortcuts 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 400
    height: 300
    title: qsTr("Hello, World!")
    flags: Qt.FramelessWindowHint

    Shortcuts { }

    AppTitleBar {
        id: appTitleBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        height: 32
        title: root.title

        windowVisibility: root.visibility

        appWindow: root

        onShowWindowMinimizedRequested: {
            root.showMinimizedWithSavePreviousState()
        }

        onToggleWindowMaximizedRequested: {
            root.toggleMaximized()
        }

        onCloseWindowRequested: {
            root.close()
        }
    }

    /*
    NotationPaintView {
        id: notationView
        anchors.fill: parent

        property NavigationPanel navigationPanel: NavigationPanel {
            name: "ScoreView"
            // section: navSec
            enabled: notationView.enabled && notationView.visible
            direction: NavigationPanel.Both
            order: 0 // Will become relevant when several scores can be opened at once
        }

        NavigationControl {
            id: fakeNavCtrl
            name: "Score"
            enabled: notationView.enabled && notationView.visible

            panel: notationView.navigationPanel
            order: 1

            onActiveChanged: {
                if (fakeNavCtrl.active) {
                    notationView.forceFocusIn()

                    if (navigationPanel.highlight) {
                        notationView.selectOnNavigationActive()
                    }
                } else {
                    notationView.focus = false
                }
            }
        }

        NavigationFocusBorder {
            navigationCtrl: fakeNavCtrl
            drawOutsideParent: false
        }

        onActiveFocusRequested: {
            fakeNavCtrl.requestActive()
        }
    }
    */
}
