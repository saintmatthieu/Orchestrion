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

import Muse.Ui as MUI // qualified, see the Muse.UiComponents import
import Muse.UiComponents as MU // qualified: its MenuItem would shadow QtQuick.Controls'
import Muse.Shortcuts as MS // qualified, see the Muse.UiComponents import
import Muse.Interactive as MI // qualified, see the Muse.UiComponents import
import Orchestrion.MuseScoreShell 1.0
import Orchestrion.OrchestrionSequencer 1.0
import Orchestrion.OrchestrionShell 1.0
import Orchestrion.OrchestrionNotation 1.0
import Orchestrion.OrchestrionOnboarding 1.0
import Orchestrion 1.0

ApplicationWindow {
    id: root
    visible: true
    visibility: Window.Maximized
    // Un-maximized. The furniture over the score (see furniture) needs the
    // room below the minimum: smaller, the score would be a postage stamp.
    width: 1024
    height: 700
    minimumWidth: 720
    minimumHeight: 480
    // Deliberately neither the file name nor a modified marker: the score's
    // own title is on the ornament, and there is nothing to save. On macOS
    // the title bar doesn't show it at all (see MacOSWindowChrome); it is
    // still what the Dock, the Window menu and Mission Control call the window.
    title: qsTrc("appshell", "Orchestrion")

    // On macOS the window keeps its native frame (traffic lights) and the
    // menus live in the OS's top menu bar, like every other Mac app. On
    // Windows/Linux we draw the frame ourselves, MuseScore-style: a custom
    // title bar with the in-window menu strip and system buttons.
    readonly property bool isMac: Qt.platform.os === "osx"
    flags: isMac ? Qt.Window
                 : Qt.Window | Qt.FramelessWindowHint | Qt.WindowSystemMenuHint

    Component.onCompleted: {
        onboardingModel.startOnboarding()
        scoreHeading.load()
    }

    // The controls over the score: the transport row and the full-screen
    // button, each its own panel. Scripts (testflow, demos) reach a control
    // by section/panel/name, e.g. Controls/Playback/Play.
    MUI.NavigationSection {
        id: controlsNavSection
        name: "Controls"
        order: 1
    }

    // The native macOS menu bar (our Qt.labs.platform wrapper around the menu
    // model, adapted from MuseScore's PlatformMenuBar). Only instantiated on
    // macOS; elsewhere the menus are rendered by AppTitleBar below.
    Loader {
        id: platformMenuBar
        active: root.isMac
        source: "MacMenuBar.qml"
        onLoaded: item.load()
    }

    // provide a rectangle for the title bar to move the window
    Rectangle {
        id: titleMoveArea
        visible: !root.isMac && root.visibility !== Window.FullScreen
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

    property var interactiveProvider: MI.InteractiveProvider {
        topParent: root

        onRequestedDockPage: function(uri, params) {
            notationPaintView.loadOrchestrionNotation()
            onPageOpened()
        }
    }

    OrchestrionOnboardingModel {
        id: onboardingModel
    }

    // Shared by the top-row grading buttons, the settings dialog and the
    // Grading menu's "Settings…" action.
    GradingModel {
        id: gradingModel
        Component.onCompleted: load()
    }

    ScoreHeadingModel {
        id: scoreHeading
    }

    ScoreShelfModel {
        id: scoreShelfModel
        Component.onCompleted: load()
        onOpenRequested: scoreShelf.open()
        onTutorialsRequested: scoreShelf.open(true)
    }

    TutorialOfferModel {
        id: tutorialOffer
        Component.onCompleted: load()
    }

    ScoreRecordingsModel {
        id: scoreRecordings
        Component.onCompleted: load()
    }

    ScoreShelfHintModel {
        id: scoreShelfHint
        Component.onCompleted: load()
    }

    KeyboardNavigationModel {
        id: keyboardNavigation
        Component.onCompleted: load()
        // Tab, or an arrow between controls, shows them like the mouse does
        onNavigated: {
            notationPaintView.controlsVisible = true
            hideControlsTimer.restart()
        }
    }

    MainWindowBridge {
        id: bridge
        window: root
    }

    // Paints the native macOS title bar in the theme's backdrop colour, so
    // that it blends with the wallpaper instead of showing the Qt palette
    // grey that MuseScore's platform styling applies. Must come after
    // MainWindowBridge: setting the bridge's window is what triggers that
    // styling, and this must overwrite it. (A Loader because the
    // MacOSWindowChrome type only exists on macOS.)
    Loader {
        active: root.isMac
        source: "MacWindowChrome.qml"
        onLoaded: item.window = root
    }

    MS.Shortcuts { }

    ColumnLayout {

        anchors.fill: parent
        spacing: 0

        // The custom title bar (Windows/Linux only, see the flags comment
        // above). A Loader rather than a hidden item so that on macOS the
        // in-window menu bar — and the second menu model it would create —
        // never exists.
        Loader {
            id: titleBar

            active: !root.isMac
            visible: active && root.visibility !== Window.FullScreen
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 30 : 0

            sourceComponent: RowLayout {
                spacing: 0

                OrchestrionTitleBar {
                    id: appTitleBar

                    Layout.fillWidth: true
                    Layout.preferredHeight: 30

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
        }

        MU.StyledViewScrollAndZoomArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            horizontalScrollbarSize: notationPaintView.horizontalScrollbarSize
            startHorizontalScrollPosition: notationPaintView.startHorizontalScrollPosition
            verticalScrollbarSize: notationPaintView.verticalScrollbarSize
            startVerticalScrollPosition: notationPaintView.startVerticalScrollPosition

            onPinchToZoom: function(scale, pos) {
                notationPaintView.pinchToZoom(scale, pos)
            }
            onScrollHorizontal: function(newPos) {
                notationPaintView.scrollHorizontal(newPos)
            }
            onScrollVertical: function(newPos) {
                notationPaintView.scrollVertical(newPos)
            }

            OrchestrionNotationPaintView {
                id: notationPaintView
                anchors.fill: parent

                // The score runs on a belt: at either end it turns around a
                // cylinder and away from the viewer. The knobs sit here so
                // they can be tried out without recompiling the view.
                // The cylinders' radius, as a fraction of the parchment's
                // height — so the rod keeps its proportion to the belt at any
                // zoom. 0.22 is the old 85 px at the default zoom.
                rollRadiusRatio: 0.22
                // How far short of the view's edge the belt turns edge-on.
                rollMargin: 10
                // How steeply the belt darkens over the turn: 1 is the plain
                // cosine, higher keeps it lit until the last moment.
                rollShadePower: 1
                // Clear parchment around the notation; its size is the
                // score's own skyline plus this, ends included.
                beltPadding: 24
                // How much of the cylinders shows past the parchment.
                rollerOverhang: 6
                // The parchment keeps as much air from the composer's name as
                // the heading puts between the title and the composer.
                titleClearance: Math.max(topOrnament.y + topOrnament.inkBottom
                                         + furniture.titleGap,
                                         videoRow.y + videoRow.height + furniture.scoreGap)
                // The score's band (see furniture, below)
                bottomClearance: height - (scoreBand.y + scoreBand.height)
                                 + furniture.scoreGap
                restingParchmentHeight: furniture.scoreBandRestingHeight
                                        - furniture.scoreBandMargins
                // Zooming in pushes the furniture out of the way, down to its
                // least (see furniture)
                tightestTitleClearance: furniture.topRowHeight + furniture.minTitleGap
                                        + topOrnament.inkBottom + furniture.titleGap
                tightestBottomClearance: furniture.scoreGap
                                         + furniture.underScoreHeight
                                         + bottomOrnament.height
                                         + 2 * furniture.minTransportGap
                                         + transportControls.implicitHeight
                                         + furniture.pedalRowHeight + furniture.minUnderPedals

                property bool controlsVisible: false

                StaffModeControls {
                    anchors.fill: parent
                    view: notationPaintView
                    navigationSection: controlsNavSection
                }

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
                    interval: 5000
                    repeat: false
                    onTriggered: {
                        notationPaintView.controlsVisible = false
                        // Hiding the controls ends their keyboard navigation:
                        // Space and the arrows are Orchestrion's again.
                        keyboardNavigation.release()
                    }
                }

                // The lining that frames the whole view, on its own border:
                // the edge of the screen in full screen, just under the menu
                // bar in a window.
                BackdropFrame {
                    anchors.fill: parent
                    opacity: notationPaintView.controlsVisible || root.visibility === Window.FullScreen ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                // The backdrop's metal lining, mirrored top and bottom; the top
                // one carries the score's title and composer. Declared first
                // so that every control and popup draws above it.
                // The window's furniture over the score, a row each, top to
                // bottom, so that none can come over another whatever the
                // window's size: the top controls' row (their own anchors),
                // the title, the score's band (the view keeps the parchment
                // in it: titleClearance, bottomClearance), the band under the
                // score (the virtual keyboard's, opening as it comes and
                // closing as it goes, the score gliding to make room), the
                // lower rule, the transport, the pedal; a demo's caption and
                // number keys have rows over the keyboard's. Rows keep
                // their room when what is in them fades. On a large window
                // the air between them is what it always was (the rules at
                // 15 and 85% of the height); it gives way to the parchment
                // as that is zoomed in, and on a small window it shrinks first,
                // then the score's band.
                ColumnLayout {
                    id: furniture
                    anchors.fill: parent
                    spacing: 0

                    // The least between the parchment, zoomed in to the most,
                    // and what is over and under it
                    readonly property real scoreGap: 10
                    readonly property real titleGap: Math.max(topOrnament.titleGap, scoreGap)
                    // What the score's band has around the parchment: the
                    // composer's name hangs into it
                    readonly property real scoreBandMargins: topOrnament.inkBottom + titleGap
                                                             - topOrnament.height + scoreGap
                    readonly property real scoreBandRestingHeight: furniture.height * 0.7
                                                                   - topOrnament.height / 2
                                                                   - videoRow.height
                                                                   - bottomOrnament.height / 2
                                                                   - underScoreHeight
                    readonly property real topRowHeight: 44
                    readonly property real minTitleGap: 4
                    // Under the lower rule: the pedals centred between it and
                    // the window's bottom, the transport between it and them.
                    // On a small window the air under the pedals goes first.
                    readonly property real underRule: Math.max(
                        0, furniture.height * 0.15 - bottomOrnament.height / 2)
                    readonly property real minTransportGap: 6
                    readonly property real minUnderPedals: 4
                    // Their ink, which their item overflows
                    readonly property real pedalRowHeight: 16
                    readonly property real underPedals: Math.max(minUnderPedals, Math.min(
                        (underRule - pedalRowHeight) / 2,
                        underRule - 2 * minTransportGap
                        - transportControls.implicitHeight - pedalRowHeight))
                    readonly property real transportGap: Math.max(
                        minTransportGap,
                        (underRule - transportControls.implicitHeight
                         - pedalRowHeight - underPedals) / 2)
                    // How far the band under the score is open: wholly while
                    // the virtual keyboard shows
                    property real lowerBandOpen: virtualKeyboard.inUse ? 1 : 0
                    Behavior on lowerBandOpen {
                        NumberAnimation { duration: 350; easing.type: Easing.InOutCubic }
                    }

                    // What takes room between the score and the lower rule
                    readonly property real underScoreHeight: demoBand.bandHeight
                                                             + lowerBandHeight * lowerBandOpen
                    // As tall as the keyboard at its full size and the air
                    // under it, no more: the rest is the score's
                    readonly property real lowerBandBottomGap: 2
                    // How far the band over the score is open: wholly while a
                    // recording is watched
                    property real videoBandOpen: scoreRecordings.currentIndex >= 0 ? 1 : 0
                    Behavior on videoBandOpen {
                        NumberAnimation { duration: 350; easing.type: Easing.InOutCubic }
                    }
                    // The composer's name hangs into it
                    readonly property real videoBandHeight: Math.min(
                        furniture.height * 0.42, (furniture.width - 300) * 9 / 16)
                        + topOrnament.inkBottom - topOrnament.height + titleGap
                    readonly property real lowerBandHeight:
                        Math.min(20, (furniture.width - 96) / 52) * 3.6
                        + virtualKeyboard.rangesHeight + lowerBandBottomGap

                    Item {
                        Layout.preferredHeight: furniture.topRowHeight
                    }
                    Item {
                        Layout.fillHeight: true
                        Layout.minimumHeight: furniture.minTitleGap
                        Layout.preferredHeight: Math.max(0, furniture.height * 0.15
                                                         - furniture.topRowHeight
                                                         - topOrnament.height / 2)
                    }

                    // The backdrop's metal lining, mirrored top and bottom; the
                    // top one carries the score's title and composer. Declared
                    // first so that every control and popup draws above it.
                    BackdropOrnament {
                        id: topOrnament
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: implicitHeight
                        viewWidth: furniture.width
                        viewHeight: furniture.height
                        title: scoreHeading.scoreTitle
                        subtitle: scoreHeading.scoreComposer
                        // The way to the other scores.
                        titleClickable: true
                        titleHintShown: notationPaintView.controlsVisible
                                        || scoreShelfHint.shown
                        onTitleClicked: scoreShelf.open()
                    }

                    // While a recording of the score is watched
                    Item {
                        id: videoRow
                        Layout.fillWidth: true
                        Layout.preferredHeight: furniture.videoBandHeight
                                                * furniture.videoBandOpen

                        VideoBand {
                            id: videoBand
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: furniture.videoBandHeight
                                    - (topOrnament.inkBottom - topOrnament.height
                                       + furniture.titleGap)
                            model: scoreRecordings
                            fullScreenHost: videoFullScreen
                            visible: parent.height > 0
                        }
                    }

                    Item {
                        id: scoreBand
                        Layout.fillHeight: true
                        // The parchment's, however far it has been zoomed in:
                        // it pushes what is over and under it
                        Layout.minimumHeight: Math.max(
                            60, furniture.scoreBandMargins + notationPaintView.parchmentHeight)
                        Layout.preferredHeight: furniture.scoreBandRestingHeight
                    }

                    // A demo's caption and number keys
                    DemoKeysBand {
                        id: demoBand
                        Layout.fillWidth: true
                        Layout.preferredHeight: bandHeight
                        spaceTop: (notationPaintView.titleClearance + notationPaintView.height
                                   - notationPaintView.bottomClearance
                                   + notationPaintView.parchmentHeight) / 2
                        spaceBottom: furniture.lowerBandOpen > 0 ? lowerBand.y : bottomOrnament.y
                        // Till it has faded out, which is when the script takes
                        // its caption away
                        covered: scoreShelf.visible
                    }

                    // While the virtual MIDI keyboard is the MIDI device in use
                    Item {
                        id: lowerBand
                        Layout.fillWidth: true
                        Layout.preferredHeight: furniture.lowerBandHeight
                                                * furniture.lowerBandOpen

                        VirtualKeyboard {
                            id: virtualKeyboard
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 48
                            anchors.rightMargin: 48
                            // Right under the score (bottomClearance keeps the
                            // parchment off), the braces under the keys
                            anchors.top: parent.top
                            height: implicitHeight
                            availableHeight: parent.height - furniture.lowerBandBottomGap
                            rangesShown: notationPaintView.controlsVisible
                        }
                    }

                    BackdropOrnament {
                        id: bottomOrnament
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: implicitHeight
                        viewWidth: furniture.width
                        viewHeight: furniture.height
                    }

                    Item {
                        Layout.fillHeight: true
                        Layout.minimumHeight: furniture.minTransportGap
                        Layout.preferredHeight: furniture.transportGap
                    }

                    // Under the backdrop's lower rule, centred, so the two read
                    // as one piece of furniture.
                    Item {
                        id: transportRow
                        Layout.fillWidth: true
                        Layout.preferredHeight: transportControls.implicitHeight

                        Row {
                            id: transportControls
                            anchors.centerIn: parent
                            spacing: 16
                            visible: opacity > 0
                            opacity: notationPaintView.controlsVisible ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 250 } }

                            PlaybackButton {
                                id: playbackRow
                                navigationSection: controlsNavSection
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                        Layout.minimumHeight: furniture.minTransportGap
                        Layout.preferredHeight: furniture.transportGap
                    }

                    // The sustain pedal. Not part of the fading controls overlay:
                    // it is feedback while playing, when the mouse is still.
                    Item {
                        id: pedalRow
                        Layout.fillWidth: true
                        Layout.preferredHeight: furniture.pedalRowHeight

                        PedalIndicator {
                            id: pedalIndicator
                            anchors.centerIn: parent
                            visible: shown
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                        Layout.minimumHeight: furniture.minUnderPedals
                        Layout.preferredHeight: furniture.underPedals
                    }
                }
                // Stays up while hovered (its panel is read there), even
                // though hovering isn't mouse activity for the overlay.
                MidiKeyboardIcon {
                    id: midiKeyboardIcon
                    x: 10
                    y: 10
                    visible: shown && opacity > 0
                    opacity: notationPaintView.controlsVisible || hovered ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                // Its own corner, mirroring the MIDI indicator's inset in the
                // opposite one.
                FullscreenButton {
                    id: fullscreenButton
                    navigationSection: controlsNavSection
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: 10
                    anchors.topMargin: 10
                    visible: opacity > 0
                    opacity: notationPaintView.controlsVisible ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                // Beside it, at the transport row's spacing
                TransportButton {
                    id: tutorialsButton
                    icon: "help.svg"
                    tooltip: Qt.platform.os === "osx" ? qsTr("Tutorials (⌘?)")
                                                      : qsTr("Tutorials (F1)")
                    navigation.name: "Tutorials"
                    navigation.panel: fullscreenButton.navigationPanel
                    navigation.order: 1
                    anchors.right: fullscreenButton.left
                    anchors.top: fullscreenButton.top
                    anchors.rightMargin: 2
                    visible: opacity > 0
                    opacity: notationPaintView.controlsVisible ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                    onClicked: scoreShelf.open(true)
                }

                // Only for a score that has videos: shows them, over the score.
                TransportButton {
                    id: watchButton
                    icon: "watch.svg"
                    tooltip: scoreRecordings.performers.length === 1
                             ? qsTr("Watch %1 play this").arg(scoreRecordings.performers[0])
                             : qsTr("Watch this played")
                    enabled: scoreRecordings.performers.length > 0
                    engaged: scoreRecordings.currentIndex >= 0
                    navigation.name: "Recordings"
                    navigation.panel: fullscreenButton.navigationPanel
                    navigation.order: 0
                    anchors.right: tutorialsButton.left
                    anchors.top: fullscreenButton.top
                    anchors.rightMargin: 2
                    visible: opacity > 0
                    opacity: enabled && notationPaintView.controlsVisible ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                    onClicked: {
                        if (scoreRecordings.currentIndex >= 0)
                            scoreRecordings.stopWatching()
                        else
                            scoreRecordings.watch(0)
                    }
                }

                // The first-class controls get the top centre: grading
                // switches between plain playing and the graded performance.
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 8
                    spacing: 8
                    visible: opacity > 0
                    opacity: notationPaintView.controlsVisible ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }

                    GradingButton {
                        model: gradingModel
                        visible: gradingModel.exposed
                    }
                }

                // Once the performer has played a score to its end: there are
                // others, and the title is the way to them. Hung under the
                // title's chevron, which it shows.
                // A while after a score opens, until taken up or closed: the
                // Getting started tutorial. Low in the view, where its keys
                // will show.
                Rectangle {
                    id: tutorialOfferCard
                    z: 100
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: parent.height * 0.72
                    width: tutorialOfferRow.implicitWidth + 28
                    height: tutorialOfferRow.implicitHeight + 20
                    radius: 8
                    color: Theme.backdrop
                    border.color: Theme.metal
                    border.width: 1
                    visible: opacity > 0
                    opacity: tutorialOffer.shown && !scoreShelf.shown ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 300 } }

                    Row {
                        id: tutorialOfferRow
                        anchors.centerIn: parent
                        spacing: 12

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("New here? Watch how to play.")
                            color: Theme.metalBright
                            font.pixelSize: 14
                        }

                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: tutorialOfferLabel.implicitWidth + 24
                            height: tutorialOfferLabel.implicitHeight + 10
                            radius: height / 2
                            color: "transparent"
                            border.color: tutorialOfferMouse.containsMouse ? Theme.metalBright
                                                                           : Theme.metal
                            Text {
                                id: tutorialOfferLabel
                                anchors.centerIn: parent
                                text: qsTr("Play")
                                color: tutorialOfferMouse.containsMouse ? Theme.metalBright
                                                                        : Theme.metal
                                font.pixelSize: 13
                            }
                            MouseArea {
                                id: tutorialOfferMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: tutorialOffer.play()
                            }
                        }

                        // Dismiss
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 22
                            height: 22
                            radius: 11
                            color: tutorialOfferCloseMouse.containsMouse ? Theme.overlay
                                                                         : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "\u00D7"
                                color: Theme.inkMuted
                                font.pixelSize: 16
                            }
                            MouseArea {
                                id: tutorialOfferCloseMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: tutorialOffer.dismiss()
                            }
                        }
                    }
                }

                Rectangle {
                    id: shelfHintCard
                    z: 100
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: topOrnament.y + topOrnament.chevronBottom + 8
                    width: shelfHintRow.implicitWidth + 28
                    height: shelfHintRow.implicitHeight + 20
                    radius: 8
                    // On the parchment: the backdrop's colour, framed in metal.
                    color: Theme.backdrop
                    border.color: Theme.metal
                    border.width: 1
                    visible: opacity > 0
                    opacity: scoreShelfHint.shown && !scoreShelf.shown ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 300 } }

                    Row {
                        id: shelfHintRow
                        anchors.centerIn: parent
                        spacing: 12

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            // The hand pointing up at the title is drawn
                            // larger than the text: at its size it is a smudge.
                            textFormat: Text.StyledText
                            text: qsTr("More scores to play: click the title %2, or press %1.")
                                  .arg(root.isMac ? "\u2318O" : "Ctrl+O")
                                  .arg("<font size=\"5\">\u261D\uFE0E</font>")
                            color: Theme.metalBright
                            font.pixelSize: 14
                        }

                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: showShelfLabel.implicitWidth + 24
                            height: showShelfLabel.implicitHeight + 10
                            radius: height / 2
                            color: "transparent"
                            border.color: showShelfMouse.containsMouse ? Theme.metalBright
                                                                       : Theme.metal
                            Text {
                                id: showShelfLabel
                                anchors.centerIn: parent
                                text: qsTr("Show me")
                                color: showShelfMouse.containsMouse ? Theme.metalBright
                                                                    : Theme.metal
                                font.pixelSize: 13
                            }
                            MouseArea {
                                id: showShelfMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: scoreShelf.open()
                            }
                        }

                        // Dismiss
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 22
                            height: 22
                            radius: 11
                            color: shelfHintCloseMouse.containsMouse ? Theme.overlay
                                                                     : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "\u00D7"
                                color: Theme.inkMuted
                                font.pixelSize: 16
                            }
                            MouseArea {
                                id: shelfHintCloseMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: scoreShelfHint.dismiss()
                            }
                        }
                    }
                }

                // Once the performer has played a score that has videos to its
                // end, graded or not. At the foot of the score's band, so that
                // it stays off the virtual keyboard.
                Rectangle {
                    id: recordingsOfferCard
                    z: 100
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: furniture.y + scoreBand.y + scoreBand.height - height - 8
                    width: recordingsOfferRow.implicitWidth + 28
                    height: recordingsOfferRow.implicitHeight + 20
                    radius: 8
                    color: Theme.backdrop
                    border.color: Theme.metal
                    border.width: 1
                    visible: opacity > 0
                    opacity: scoreRecordings.offerShown && !tutorialOffer.shown
                             && !scoreShelf.shown ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 300 } }

                    Row {
                        id: recordingsOfferRow
                        anchors.centerIn: parent
                        spacing: 12

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: scoreRecordings.performers.length === 1
                                  ? qsTr("Watch %1 play this.").arg(scoreRecordings.performers[0])
                                  : qsTr("See how others play this.")
                            color: Theme.metalBright
                            font.pixelSize: 14
                        }

                        Rectangle {
                            id: recordingsOfferWatch
                            anchors.verticalCenter: parent.verticalCenter
                            width: recordingsOfferLabel.implicitWidth + 24
                            height: recordingsOfferLabel.implicitHeight + 10
                            radius: height / 2
                            color: "transparent"
                            border.color: recordingsOfferMouse.containsMouse ? Theme.metalBright
                                                                             : Theme.metal
                            Text {
                                id: recordingsOfferLabel
                                anchors.centerIn: parent
                                text: qsTr("Watch")
                                color: recordingsOfferMouse.containsMouse ? Theme.metalBright
                                                                          : Theme.metal
                                font.pixelSize: 13
                            }
                            MouseArea {
                                id: recordingsOfferMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: scoreRecordings.watch(0)
                            }
                        }

                        // Dismiss
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 22
                            height: 22
                            radius: 11
                            color: recordingsOfferCloseMouse.containsMouse ? Theme.overlay
                                                                           : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "\u00D7"
                                color: Theme.inkMuted
                                font.pixelSize: 16
                            }
                            MouseArea {
                                id: recordingsOfferCloseMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: scoreRecordings.closeOffer()
                            }
                        }
                    }
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
                OrchestrionToolTip {
                    id: noteInfoToolTip
                    z: 102
                    delay: 0
                    visible: notationPaintView.hoveredNoteInfo.length > 0
                    text: notationPaintView.hoveredNoteInfo
                    // Below-right of the cursor by default; a timing gauge's
                    // tip anchors beside the onset's noteheads instead —
                    // left of an early note's coloured copy, right of a late
                    // one's — so it covers neither the black nor the
                    // coloured notes. Kept where it was while it fades out:
                    // the placement resets as the note is left.
                    Binding on x {
                        when: notationPaintView.hoveredNoteInfo.length > 0
                        restoreMode: Binding.RestoreNone
                        value: notationPaintView.hoveredNoteInfoPlacement === 1
                               ? notationPaintView.hoveredNoteInfoPos.x - noteInfoToolTip.width
                               : notationPaintView.hoveredNoteInfoPos.x
                                 + (notationPaintView.hoveredNoteInfoPlacement === 2 ? 0 : 16)
                    }
                    Binding on y {
                        when: notationPaintView.hoveredNoteInfo.length > 0
                        restoreMode: Binding.RestoreNone
                        value: notationPaintView.hoveredNoteInfoPlacement === 0
                               ? notationPaintView.hoveredNoteInfoPos.y + 16
                               : notationPaintView.hoveredNoteInfoPos.y - noteInfoToolTip.height / 2
                    }
                }

                // The grading configuration dialog, opened from the Grading
                // menu's "Settings…" item.
                GradingSettingsPopup {
                    id: gradingPopup
                    z: 105
                    model: gradingModel
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 3
                }

                Connections {
                    target: gradingModel
                    function onOpenSettingsRequested() {
                        gradingPopup.open()
                    }
                }

                // End-of-piece banner: the take's timing score, raised by the
                // paint view when the last notes are released. Closes with
                // the cross, Escape, or by starting to play again.
                Rectangle {
                    id: scoreBanner
                    z: 103

                    // Keep the last real score during the fade-out (the
                    // properties reset the moment it is dismissed).
                    property int shownScore: 0
                    property string shownBreakdown: ""
                    property var shownMetrics: []
                    // The metrics panel starts collapsed and folds back for
                    // the next take.
                    property bool expanded: false
                    Connections {
                        target: notationPaintView
                        function onFinalScoreChanged() {
                            if (notationPaintView.finalScore >= 0) {
                                scoreBanner.shownScore = notationPaintView.finalScore
                                scoreBanner.shownBreakdown = notationPaintView.finalScoreBreakdown
                                scoreBanner.shownMetrics = notationPaintView.finalScoreMetrics
                            } else {
                                scoreBanner.expanded = false
                            }
                        }
                    }

                    anchors.horizontalCenter: parent.horizontalCenter
                    y: parent.height / 5
                    width: bannerColumn.width + 96
                    height: bannerColumn.height + 40
                    radius: scoreBanner.expanded ? 24 : height / 2
                    Behavior on height { NumberAnimation { duration: 200 } }
                    Behavior on width { NumberAnimation { duration: 200 } }
                    color: Theme.overlay
                    border.color: Theme.metalBright
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

                    Column {
                        id: bannerColumn
                        anchors.centerIn: parent
                        spacing: 4

                        Text {
                            id: scoreBannerText
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("You scored %1 !").arg(scoreBanner.shownScore)
                            color: Theme.metalBright
                            font.pixelSize: 40
                            font.bold: true
                        }

                        Text {
                            visible: scoreBanner.shownBreakdown.length > 0
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: scoreBanner.shownBreakdown
                            color: Theme.inkMuted
                            font.pixelSize: 17
                        }

                        // The expander: what the score was made of.
                        Text {
                            id: expandArrow
                            visible: scoreBanner.shownMetrics.length > 0
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: scoreBanner.expanded ? "▴" : "▾"
                            color: Theme.inkMuted
                            font.pixelSize: 18

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -10
                                cursorShape: Qt.PointingHandCursor
                                onClicked: scoreBanner.expanded = !scoreBanner.expanded
                            }
                        }

                        Column {
                            id: metricsColumn
                            visible: scoreBanner.expanded
                                     && scoreBanner.shownMetrics.length > 0
                            anchors.horizontalCenter: parent.horizontalCenter
                            topPadding: 6
                            spacing: 6

                            Repeater {
                                model: scoreBanner.shownMetrics

                                // Intrinsic sizing throughout: the banner
                                // sizes itself from this column, so a row
                                // must never take its width from it.
                                Row {
                                    spacing: 16

                                    Column {
                                        width: 220
                                        spacing: 1

                                        Text {
                                            text: modelData.label
                                            color: Theme.accent
                                            font.pixelSize: 15
                                        }

                                        Text {
                                            text: modelData.detail
                                            color: Theme.inkMuted
                                            font.pixelSize: 12
                                        }
                                    }

                                    Text {
                                        width: 40
                                        horizontalAlignment: Text.AlignRight
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.score
                                        color: Theme.metalBright
                                        font.pixelSize: 20
                                        font.bold: true
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        text: "✕"
                        color: Theme.inkFaint
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

                // Every score one can open, over everything else in the view.
                ScoreShelf {
                    id: scoreShelf
                    anchors.fill: parent
                    z: 150
                    model: scoreShelfModel
                    onShownChanged: {
                        if (shown) {
                            // Found: no more need for the hint.
                            scoreShelfHint.dismiss()
                            // The keys are the shelf's now: Space and Return
                            // would otherwise press the control underneath.
                            keyboardNavigation.release()
                        }
                    }
                }

                // Post-take tuning panel: the tempo model's smoothing memory.
                // Dragging the slider re-fits the finished take live, so the
                // effect on the curve, marks and warp is directly observable.
                Rectangle {
                    id: smoothingTuner
                    z: 104

                    anchors.horizontalCenter: parent.horizontalCenter
                    // Above the transport, not to hide it.
                    y: transportRow.y - height - 8
                    width: tunerRow.width + 48
                    height: tunerRow.height + 24
                    radius: height / 2
                    color: Theme.overlay
                    border.color: Theme.metalBright
                    border.width: 1
                    visible: opacity > 0
                    opacity: notationPaintView.smoothingTunerVisible ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }

                    Row {
                        id: tunerRow
                        anchors.centerIn: parent
                        spacing: 12

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("tempo smoothing: %1")
                                      .arg(notationPaintView.tempoSmoothing.toFixed(2))
                            color: Theme.inkMuted
                            font.pixelSize: 15
                        }

                        Slider {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 220
                            from: 0.3
                            to: 0.95
                            value: notationPaintView.tempoSmoothing
                            onMoved: notationPaintView.tempoSmoothing = value
                        }
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

    // A video gone full screen, from its player's own button
    property int visibilityBeforeVideo: Window.Windowed
    Item {
        id: videoFullScreen
        anchors.fill: parent
        z: 900
        visible: videoBand.fullScreen
        Rectangle { anchors.fill: parent; color: "black" }
    }
    Connections {
        target: videoBand
        function onFullScreenChanged() {
            if (videoBand.fullScreen) {
                root.visibilityBeforeVideo = root.visibility
                root.showFullScreen()
            } else if (root.visibilityBeforeVideo === Window.Maximized) {
                root.showMaximized()
            } else if (root.visibilityBeforeVideo !== Window.FullScreen) {
                root.showNormal()
            }
        }
    }

    NotationPaintViewLoaderModel {
        id: notationPaintViewLoaderModel
        Component.onCompleted: {
            notationPaintViewLoaderModel.init()
        }
    }

    // Over everything else in the window
    DemoKeysOverlay {
        anchors.fill: parent
        z: 1000
        topInset: titleBar.visible ? titleBar.height : 0
        topRowHeight: furniture.topRowHeight
        shelfShown: scoreShelf.shown
    }

    // The window's edges and corners resize it: frameless, it has no frame
    // of its own to (macOS keeps the native one)
    Repeater {
        model: [
            { edges: Qt.TopEdge, cursor: Qt.SizeVerCursor },
            { edges: Qt.BottomEdge, cursor: Qt.SizeVerCursor },
            { edges: Qt.LeftEdge, cursor: Qt.SizeHorCursor },
            { edges: Qt.RightEdge, cursor: Qt.SizeHorCursor },
            { edges: Qt.TopEdge | Qt.LeftEdge, cursor: Qt.SizeFDiagCursor },
            { edges: Qt.BottomEdge | Qt.RightEdge, cursor: Qt.SizeFDiagCursor },
            { edges: Qt.TopEdge | Qt.RightEdge, cursor: Qt.SizeBDiagCursor },
            { edges: Qt.BottomEdge | Qt.LeftEdge, cursor: Qt.SizeBDiagCursor }
        ]

        MouseArea {
            required property var modelData
            readonly property real grip: 6
            readonly property bool atTop: modelData.edges & Qt.TopEdge
            readonly property bool atBottom: modelData.edges & Qt.BottomEdge
            readonly property bool atLeft: modelData.edges & Qt.LeftEdge
            readonly property bool atRight: modelData.edges & Qt.RightEdge
            // A corner a little bigger than the edges, to be easier to catch
            readonly property bool corner: (atTop || atBottom) && (atLeft || atRight)

            visible: !root.isMac && root.visibility === Window.Windowed
            z: 2000
            x: atRight ? root.width - width : 0
            y: atBottom ? root.height - height : 0
            width: atLeft || atRight ? (corner ? 3 * grip : grip) : root.width
            height: atTop || atBottom ? (corner ? 3 * grip : grip) : root.height
            cursorShape: modelData.cursor
            onPressed: root.startSystemResize(modelData.edges)
        }
    }

    DemoCursorOverlay {
        anchors.fill: parent
        z: 1000
    }
}
