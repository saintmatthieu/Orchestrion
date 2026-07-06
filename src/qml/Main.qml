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
import Orchestrion.OrchestrionSequencer 1.0
import Orchestrion.OrchestrionShell 1.0
import Orchestrion.OrchestrionNotation 1.0
import Orchestrion.OrchestrionOnboarding 1.0

ApplicationWindow {
    id: root
    visible: true
    visibility: Window.Maximized
    width: 800
    height: 350
    title: titleProvider.title
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowSystemMenuHint

    Component.onCompleted: {
        onboardingModel.startOnboarding()
        titleProvider.load()
    }

    // provide a rectangle for the title bar to move the window
    Rectangle {
        id: titleMoveArea
        visible: root.visibility !== Window.FullScreen
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

        RowLayout {

            id: titleBar

            visible: root.visibility !== Window.FullScreen
            spacing: 0
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 30 : 0

            OrchestrionIcon {
                id: orchestrionIcon
                Layout.preferredWidth: 30 + orchestrionIcon.leftPadding
                Layout.preferredHeight: 30
            }

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
        }

        NotationScrollAndZoomArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            OrchestrionNotationPaintView {
                id: notationPaintView
                anchors.fill: parent

                property bool controlsVisible: false

                onMouseActivity: {
                    notationPaintView.controlsVisible = true
                    hideControlsTimer.restart()
                }

                onContextMenuRequested: function(position) {
                    loopContextMenu.popup(position.x, position.y)
                }

                Menu {
                    id: loopContextMenu

                    MenuItem {
                        text: "Set loop start (I)"
                        enabled: notationPaintView.contextMenuHasTarget
                        onTriggered: notationPaintView.contextMenuSetLoopStart()
                    }
                    MenuItem {
                        text: "Set loop end (O)"
                        enabled: notationPaintView.contextMenuHasTarget
                        onTriggered: notationPaintView.contextMenuSetLoopEnd()
                    }
                    MenuSeparator { }
                    MenuItem {
                        text: "Clear loop"
                        onTriggered: notationPaintView.clearLoop()
                    }
                }

                Timer {
                    id: hideControlsTimer
                    interval: 2000
                    repeat: false
                    onTriggered: notationPaintView.controlsVisible = false
                }

                GestureControllerSelectionPopup {
                    x: 10
                    y: 10
                    id: selectionPopup
                    visible: opacity > 0
                    opacity: notationPaintView.controlsVisible ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                MidiDeviceActivityPopup {
                    x: selectionPopup.x + selectionPopup.width + 10
                    y: selectionPopup.y
                }

                Row {
                    id: topRightControls
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: 8
                    anchors.topMargin: 8
                    spacing: 16
                    visible: opacity > 0
                    opacity: notationPaintView.controlsVisible ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }

                    PlaybackButton {
                        id: playbackRow
                    }

                    FullscreenButton {
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Beginner help: number-key tooltip + "Show me!" animation.
                // Stays put (not tied to the fading controls overlay).
                NumberKeysHelp {
                    anchors.fill: parent
                    z: 100
                }

                // Transient credit shown when a score that must be attributed
                // (e.g. a Creative-Commons transcription) is opened.
                AttributionToast {
                    anchors.fill: parent
                    z: 101
                }

                // Debug aid: note info shown on hover, toggled from the
                // Advanced menu ("Show note info on hover"). The paint view
                // leaves hoveredNoteInfo empty unless the feature is enabled
                // and a note is under the cursor.
                ToolTip {
                    id: noteInfoToolTip
                    z: 102
                    delay: 0
                    visible: notationPaintView.hoveredNoteInfo.length > 0
                    text: notationPaintView.hoveredNoteInfo
                    x: notationPaintView.hoveredNoteInfoPos.x + 16
                    y: notationPaintView.hoveredNoteInfoPos.y + 16
                }

                // End-of-piece banner: the take's timing score, raised by the
                // paint view when the last notes are released. Closes with
                // the cross, Escape, or by starting to play again.
                Rectangle {
                    id: scoreBanner
                    z: 103

                    // Keep the last real score during the fade-out (the
                    // property returns to -1 the moment it is dismissed).
                    property int shownScore: 0
                    Connections {
                        target: notationPaintView
                        function onFinalScoreChanged() {
                            if (notationPaintView.finalScore >= 0)
                                scoreBanner.shownScore = notationPaintView.finalScore
                        }
                    }

                    anchors.horizontalCenter: parent.horizontalCenter
                    y: parent.height / 5
                    width: scoreBannerText.implicitWidth + 96
                    height: scoreBannerText.implicitHeight + 44
                    radius: height / 2
                    color: "#E8241811"
                    border.color: "#E5B84B"
                    border.width: 2
                    visible: opacity > 0
                    opacity: notationPaintView.finalScore >= 0 ? 1 : 0
                    scale: notationPaintView.finalScore >= 0 ? 1 : 0.7
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                    Behavior on scale {
                        NumberAnimation {
                            duration: 350
                            easing.type: Easing.OutBack
                        }
                    }

                    Text {
                        id: scoreBannerText
                        anchors.centerIn: parent
                        text: qsTr("You scored %1 !").arg(scoreBanner.shownScore)
                        color: "#E5B84B"
                        font.pixelSize: 40
                        font.bold: true
                    }

                    Text {
                        text: "✕"
                        color: "#B8A88F"
                        font.pixelSize: 16
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: 10
                        anchors.rightMargin: 14

                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -8
                            cursorShape: Qt.PointingHandCursor
                            onClicked: notationPaintView.dismissFinalScore()
                        }
                    }

                    Shortcut {
                        sequence: "Escape"
                        enabled: scoreBanner.visible
                        onActivated: notationPaintView.dismissFinalScore()
                    }
                }
            }
        }

        // Debug aid: real-time tempo-model visualization, toggled from the
        // Advanced menu ("Show tempo visualization"). Sits beneath the score
        // with a small margin; reserves no space when hidden.
        TempoVisualizationView {
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 120 : 0
            Layout.margins: visible ? 8 : 0
            visible: notationPaintView.tempoVisualizationEnabled
            model: notationPaintView.tempoVizModel
        }
    }

    NotationPaintViewLoaderModel {
        id: notationPaintViewLoaderModel
        Component.onCompleted: {
            notationPaintViewLoaderModel.init()
        }
    }
}
