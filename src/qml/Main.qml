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
import MuseScore.NotationScene 1.0
import MuseScore.AppShell 1.0
import MuseScore.Shortcuts 1.0
import Orchestrion.OrchestrionShell 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: qsTr("Hello, World!")
    // flags: Qt.FramelessWindowHint

    property var interactiveProvider: InteractiveProvider {
        topParent: root

        onRequestedDockPage: function(uri, params) {
            notationPaintView.item.load()
        }
    }

    Shortcuts { }

    ColumnLayout {

        Layout.fillWidth: true
        Layout.fillHeight: true

        AppTitleBar {
            id: appTitleBar

            width: root.width
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

        Component {
            id: notationPaintViewComponent

            NotationPaintView {
                id: notationView
                width: root.width
                height: root.height - appTitleBar.height
            }
        }
    }

    Loader {
        id: notationPaintView
        onLoaded: {
            console.log("Yo: notationPaintView loaded")
            item.anchors.top = appTitleBar.bottom
            item.anchors.left = parent.left
            item.anchors.right = parent.right
            item.anchors.bottom = parent.bottom
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
            if (notationPaintView.sourceComponent !== notationPaintViewComponent) {
                console.log("Yo: NotationPaintViewLoaderModel NotationPaintViewReady: set sourceComponent")
                notationPaintView.sourceComponent = notationPaintViewComponent
            }
        }
    }
}
