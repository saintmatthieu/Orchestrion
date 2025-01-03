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

import Muse.Ui 1.0
import MuseScore.NotationScene 1.0
import MuseScore.AppShell 1.0
import Muse.Shortcuts 1.0
import Orchestrion.OrchestrionShell 1.0
import Orchestrion.OrchestrionNotation 1.0
import Orchestrion.OrchestrionOnboarding 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 350
    title: titleProvider.title
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowSystemMenuHint

    onActiveChanged: {
        console.log("Active changed: " + active)
        if (active)
            onboardingModel.onGainedFocus()
    }

    Component.onCompleted: {
        onboardingModel.startOnboarding()
        titleProvider.load()
    }

    // provide a rectangle for the title bar to move the window
    Rectangle {
        id: titleMoveArea
        width: parent.width
        height: 32

        MouseArea {
            id: dragArea
            anchors.fill: parent
            onPressed: root.startSystemMove()
        }
    }

    function toggleMaximized() {
        if (root.visibility === Window.Maximized) {
            root.showNormal()
        } else {
            root.showMaximized()
        }
    }

    property var interactiveProvider: InteractiveProvider {
        topParent: root

        onRequestedDockPage: function(uri, params) {
            notationPaintView.loadOrchestrionNotation()
            onPageOpened()
        }
    }

    OrchestrionOnboardingModel {
        id: onboardingModel
    }

    OrchestrionWindowTitleProvider {
        id: titleProvider
    }

    MainWindowBridge {
        id: bridge
        window: root
        //! NOTE These properties of QWindow (of which ApplicationWindow is derived)
        //!      are not available in QML, so we access them via MainWindowBridge
        filePath: titleProvider.filePath
        fileModified: titleProvider.fileModified
    }

    Shortcuts { }

    ColumnLayout {

        anchors.fill: parent
        spacing: 0

        AppTitleBar {
            id: appTitleBar

            Layout.fillWidth: true
            Layout.preferredHeight: 30

            title: root.title

            windowVisibility: root.visibility

            appWindow: root

            onShowWindowMinimizedRequested: {
                bridge.showMinimizedWithSavePreviousState()
            }

            onToggleWindowMaximizedRequested: {
                root.toggleMaximized()
            }

            onCloseWindowRequested: {
                root.close()
            }
        }

        NotationScrollAndZoomArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            OrchestrionNotationPaintView {
                id: notationPaintView
                anchors.fill: parent
            }
        }
    }

    NotationPaintViewLoaderModel {
        id: notationPaintViewLoaderModel
        Component.onCompleted: {
            console.log("Yo: NotationPaintViewLoaderModel completed")
            notationPaintViewLoaderModel.init()
        }
        onNotationPaintViewReady: {
            console.log("Yo: NotationPaintViewLoaderModel NotationPaintViewReady")
        }
    }
}
