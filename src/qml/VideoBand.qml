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
import QtWebEngine
import Qt5Compat.GraphicalEffects

import Orchestrion 1.0

/**
 * The score's recordings: a thumbnail tucked behind the parchment, its top
 * peeking out over the score's top edge with "Watch … play this." on it,
 * while the controls show, near the left cylinder, where the eyes go first.
 * Under the pointer it comes all the way out, a few pixels over the
 * parchment; clicked, it grows where it is, with the band that opens between
 * the title and the score and pushes the score down (smaller, still under it,
 * to follow the music), playing, with the score's other recordings beside it;
 * from there it can go full screen.
 *
 * It grows with the band (`progress`) and shrinks back the same way. YouTube's
 * player runs in a page of the website (player.html); a video its owner
 * doesn't let play elsewhere is offered in the browser instead.
 */
Item {
    id: root

    /** A ScoreRecordingsModel. */
    property var model
    /** Where the video goes full screen. */
    property Item fullScreenHost
    /** How far the band is open, from 0 (tucked) to 1 (opened). */
    property real progress: 0
    /** Whether the window's controls show: the thumbnail peeks out only then. */
    property bool controlsVisible: false
    /**
     * The parchment's top edge, which the thumbnail is tucked behind, and the
     * left cylinder's right edge, which it keeps clear of, moving with it.
     */
    property real parchmentTop: 0
    property real leftBarrelRight: 0
    /** How high it may come out: up to the title's bottom. */
    property real titleBottom: 0
    /** Opened: its height. */
    property real openHeight: 0

    readonly property bool fullScreen: internal.fullScreen

    function toggleFullScreen() {
        if (internal.playerFullScreen)
            playerLoader.item.triggerWebAction(WebEngineView.ExitFullScreen)
        else
            internal.fullScreen = !internal.fullScreen
    }

    QtObject {
        id: internal
        property bool fullScreen: false
        // The player asked for it (its own button), and must be told when it
        // ends.
        property bool playerFullScreen: false
        property bool unavailable: false
        // The player has started: its picture replaces the thumbnail.
        property bool started: false
        // The video being watched, or, while the band closes, the last one.
        property url playerUrl
        readonly property bool hasRecordings: root.model.performers.length > 0
    }

    Connections {
        target: root.model
        function onCurrentChanged() {
            if (root.model.currentIndex < 0) {
                if (internal.playerFullScreen)
                    playerLoader.item.triggerWebAction(WebEngineView.ExitFullScreen)
                internal.fullScreen = false
                if (playerLoader.item)
                    playerLoader.item.runJavaScript("window.pausePlayer && pausePlayer()")
                return
            }
            internal.unavailable = false
            internal.started = false
            internal.playerUrl = root.model.playerUrl
        }
        function onPauseRequested() {
            if (playerLoader.item)
                playerLoader.item.runJavaScript("window.pausePlayer && pausePlayer()")
        }
    }

    Shortcut {
        sequence: "Escape"
        enabled: internal.fullScreen
        onActivated: root.toggleFullScreen()
    }

    // Tucked: its size (wide enough for what it says), and how much of it
    // shows over the parchment: all of it under the pointer or while watched
    // (so that it opens from there), and the air over the parchment it then
    // stands in, as when opened.
    readonly property real tuckedWidth: Math.max(200, caption.implicitWidth + 24)
    readonly property real tuckedHeight: tuckedWidth * 9 / 16
    readonly property real captionHeight: 28
    readonly property real gap: 6
    // Between the title and the score, less some air; it hides when the
    // score, zoomed in, leaves too little.
    readonly property real room: parchmentTop - titleBottom - 4
    property real peek: !internal.hasRecordings || room < captionHeight ? 0
                        : root.model.currentIndex >= 0 ? tuckedHeight + gap
                        : cardMouse.containsMouse ? Math.min(tuckedHeight + gap, room)
                        : root.controlsVisible ? captionHeight : 0
    Behavior on peek { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

    // Behind the parchment: what is below its top edge is cut off.
    Item {
        id: tuck
        width: root.width
        height: root.parchmentTop
        clip: true

        Item {
            id: card
            readonly property real p: root.progress
            // How far its bottom is over the parchment's top edge: under it
            // while tucked
            readonly property real lift: (root.peek - root.tuckedHeight) * (1 - p)
                                         + root.gap * p
            // How far out from behind the parchment, 0 to 1
            readonly property real out: Math.max(0, Math.min(1, lift / root.gap))
            // Its top corners round; the bottom ones too once out.
            readonly property real radius: 6
            readonly property real bottomRadius: radius * out
            x: root.leftBarrelRight + 30
            width: root.tuckedWidth * (1 - p) + root.openHeight * 16 / 9 * p
            height: width * 9 / 16
            y: root.parchmentTop - lift - height
            visible: root.peek > 0 || p > 0

            // Its outline, which the picture is cut to
            Rectangle {
                id: cardShape
                anchors.fill: parent
                topLeftRadius: card.radius
                topRightRadius: card.radius
                bottomLeftRadius: card.bottomRadius
                bottomRightRadius: card.bottomRadius
                visible: false
            }

            Item {
                id: cardContent
                anchors.fill: parent
                layer.enabled: true
                layer.effect: OpacityMask { maskSource: cardShape }

                Rectangle {
                    anchors.fill: parent
                    color: "black"
                }

                Image {
                    anchors.fill: parent
                    source: root.model.thumbnailUrl
                    sourceSize.width: 320
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    visible: !internal.started
                }

                Item {
                    id: playerFrame
                    anchors.fill: parent

                    Item {
                        parent: internal.fullScreen ? root.fullScreenHost : playerFrame
                        anchors.fill: parent

                        Loader {
                            id: playerLoader
                            anchors.fill: parent
                            active: root.model.currentIndex >= 0
                                    || (root.progress > 0 && internal.playerUrl.toString().length > 0)
                            sourceComponent: WebEngineView {
                                url: internal.playerUrl
                                backgroundColor: "black"
                                settings.playbackRequiresUserGesture: false
                                settings.fullScreenSupportEnabled: true

                                onFullScreenRequested: function(request) {
                                    request.accept()
                                    internal.playerFullScreen = request.toggleOn
                                    internal.fullScreen = request.toggleOn
                                }
                                onJavaScriptConsoleMessage: function(level, message) {
                                    if (message.startsWith("orchestrion-player:error:"))
                                        internal.unavailable = true
                                    else if (message.startsWith("orchestrion-player:state:"))
                                        internal.started = true
                                }
                                // The player's links (its logo, the title, ...)
                                // leave it: those are for the browser.
                                onNewWindowRequested: function(request) {
                                    Qt.openUrlExternally(request.requestedUrl)
                                }
                                onNavigationRequested: function(request) {
                                    if (request.isMainFrame && request.url.toString().split("?")[0]
                                            !== internal.playerUrl.toString().split("?")[0]) {
                                        request.reject()
                                        Qt.openUrlExternally(request.url)
                                    }
                                }
                            }
                        }
                    }
                }

                // What it is, on the part that peeks out
                Rectangle {
                    width: parent.width
                    height: root.captionHeight
                    opacity: 1 - card.p
                    visible: opacity > 0
                    color: Theme.backdrop
                    Text {
                        id: caption
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        text: root.model.performers.length === 1
                              ? qsTr("Watch %1 play this.").arg(root.model.performers[0])
                              : qsTr("Watch this played.")
                        color: Theme.metalBright
                        font.pixelSize: 13
                    }
                }

                // A video that can't play here
                Rectangle {
                    anchors.fill: parent
                    visible: internal.unavailable
                    color: Theme.backdrop

                    Column {
                        anchors.centerIn: parent
                        width: parent.width - 16
                        spacing: 8

                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
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

            // The frame, over the picture's edge: open at the bottom till the
            // card is out from behind the parchment, which it goes on under
            Item {
                anchors.fill: parent
                clip: true
                Rectangle {
                    width: parent.width
                    height: parent.height + 4 * (1 - card.out)
                    color: "transparent"
                    border.color: Theme.metal
                    border.width: 1
                    topLeftRadius: card.radius
                    topRightRadius: card.radius
                    bottomLeftRadius: card.bottomRadius
                    bottomRightRadius: card.bottomRadius
                }
            }

            // Tucked: comes out under the pointer, opens when clicked. Opened,
            // the player has the clicks.
            MouseArea {
                id: cardMouse
                anchors.fill: parent
                enabled: root.model.currentIndex < 0 && root.progress === 0
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.model.watch(0)
            }
        }
    }

    // The score's recordings, and the ways out
    Rectangle {
        x: card.x + card.width + 16
        y: card.y
        width: Math.min(root.width - x - 16, recordingsColumn.implicitWidth + 24)
        height: recordingsColumn.height + 20
        radius: 8
        color: Theme.backdrop
        border.color: Theme.metal
        border.width: 1
        visible: opacity > 0
        opacity: root.progress

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
                    width: Math.min(implicitWidth, root.width / 4)
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

            Text {
                text: qsTr("Full screen")
                color: Theme.accent
                font.pixelSize: 13
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggleFullScreen()
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
