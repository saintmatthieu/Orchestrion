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
import QtQuick.Layouts
import QtWebEngine

import Orchestrion 1.0

/**
 * The recording being watched, in the app, with the score's others to choose
 * from. YouTube's player runs in a page of the website (player.html); a video
 * its owner doesn't let play elsewhere is offered in the browser instead.
 */
Item {
    id: root

    /** A ScoreRecordingsModel. */
    property var model
    /** Where the video goes when its player goes full screen. */
    property Item fullScreenHost

    readonly property bool fullScreen: internal.fullScreen

    QtObject {
        id: internal
        property bool fullScreen: false
        property bool unavailable: false
    }

    Connections {
        target: root.model
        function onCurrentChanged() {
            internal.unavailable = false
            if (root.model.currentIndex < 0)
                internal.fullScreen = false
        }
        function onPauseRequested() {
            if (playerLoader.item)
                playerLoader.item.runJavaScript("window.pausePlayer && pausePlayer()")
        }
    }

    Shortcut {
        sequence: "Escape"
        enabled: internal.fullScreen
        onActivated: playerLoader.item.triggerWebAction(WebEngineView.ExitFullScreen)
    }

    Item {
        id: playerArea
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: height * 16 / 9

        Item {
            parent: internal.fullScreen ? root.fullScreenHost : playerArea
            anchors.fill: parent

            Loader {
                id: playerLoader
                anchors.fill: parent
                active: root.model.currentIndex >= 0
                sourceComponent: WebEngineView {
                    url: root.model.playerUrl
                    backgroundColor: "black"
                    settings.playbackRequiresUserGesture: false
                    settings.fullScreenSupportEnabled: true

                    onFullScreenRequested: function(request) {
                        request.accept()
                        internal.fullScreen = request.toggleOn
                    }
                    onJavaScriptConsoleMessage: function(level, message) {
                        if (message.startsWith("orchestrion-player:error:"))
                            internal.unavailable = true
                    }
                    // The player's links (its logo, the title, ...) leave it:
                    // those are for the browser.
                    onNewWindowRequested: function(request) {
                        Qt.openUrlExternally(request.requestedUrl)
                    }
                    onNavigationRequested: function(request) {
                        if (request.isMainFrame && request.url.toString().split("?")[0]
                                !== root.model.playerUrl.toString().split("?")[0]) {
                            request.reject()
                            Qt.openUrlExternally(request.url)
                        }
                    }
                }
            }
        }

        // A video that can't play here
        Rectangle {
            anchors.fill: parent
            visible: internal.unavailable
            color: Theme.backdrop
            border.color: Theme.metal

            Column {
                anchors.centerIn: parent
                spacing: 12

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("This recording can only be watched on YouTube.")
                    color: Theme.metalBright
                    font.pixelSize: 15
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Watch it on YouTube")
                    color: Theme.accent
                    font.pixelSize: 14
                    font.underline: true
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.model.watchInBrowser()
                    }
                }
            }
        }
    }

    // The score's recordings, and the way out
    Rectangle {
        anchors.left: playerArea.right
        anchors.top: playerArea.top
        anchors.leftMargin: 16
        width: Math.min(parent.width - playerArea.x - playerArea.width - 32,
                        recordingsColumn.implicitWidth + 24)
        height: recordingsColumn.height + 20
        radius: 8
        color: Theme.backdrop
        border.color: Theme.metal
        border.width: 1

        Column {
            id: recordingsColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            anchors.topMargin: 10
            spacing: 8

            Repeater {
                model: root.model.performers

                Text {
                    required property int index
                    required property string modelData
                    width: Math.min(implicitWidth, recordingsColumn.parent.parent.width / 4)
                    elide: Text.ElideRight
                    text: modelData
                    color: index === root.model.currentIndex ? Theme.metalBright : Theme.metal
                    font.pixelSize: 14
                    font.bold: index === root.model.currentIndex
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.model.watch(parent.index)
                    }
                }
            }

            // Always: YouTube doesn't always say when it refuses a video.
            Text {
                text: qsTr("Open on YouTube")
                color: Theme.accent
                font.pixelSize: 13
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.model.watchInBrowser()
                }
            }

            Text {
                text: qsTr("Close")
                color: Theme.inkMuted
                font.pixelSize: 13
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.model.stopWatching()
                }
            }
        }
    }
}
