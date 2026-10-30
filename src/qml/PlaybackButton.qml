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

import Muse.Ui as MUI // qualified, like Main.qml's

import Orchestrion 1.0
import Orchestrion.OrchestrionShell 1.0

// The transport row at the top right of the score view. Play and loop show
// whether they are running by glowing, so neither ever swaps its glyph.
Row {
    id: root

    spacing: 2

    /** The navigation section the row's panel belongs to. */
    property MUI.NavigationSection navigationSection

    MUI.NavigationPanel {
        id: navPanel
        name: "Playback"
        section: root.navigationSection
        order: 1
        direction: MUI.NavigationPanel.Horizontal
        enabled: root.enabled // faded out too: Tab brings them back (Main.qml)
        accessible.name: qsTr("Playback")
    }

    PlaybackButtonModel {
        id: playbackModel
    }

    Component.onCompleted: {
        playbackModel.load()
    }

    TransportButton {
        icon: "rewind.svg"
        tooltip: qsTr("Rewind (Home)")
        navigation.name: "Rewind"
        navigation.panel: navPanel
        navigation.order: 1
        onClicked: playbackModel.rewind()
    }

    TransportButton {
        icon: "previous.svg"
        tooltip: qsTr("Previous (Left)")
        navigation.name: "Previous"
        navigation.panel: navPanel
        navigation.order: 2
        onClicked: playbackModel.backStep()
    }

    TransportButton {
        icon: "play.svg"
        tooltip: qsTr("Play/Pause (Space)")
        navigation.name: "Play"
        navigation.panel: navPanel
        navigation.order: 3
        engaged: playbackModel.isPlaying
        onClicked: playbackModel.togglePlay()
    }

    TransportButton {
        icon: "stop.svg"
        tooltip: qsTr("Stop")
        navigation.name: "Stop"
        navigation.panel: navPanel
        navigation.order: 4
        onClicked: playbackModel.stop()
    }

    // Drawn from the font's own right guillemet rather than the previous icon
    // rotated, which would carry the ring's stress round with it.
    TransportButton {
        icon: "next.svg"
        tooltip: qsTr("Next (Right)")
        navigation.name: "Next"
        navigation.panel: navPanel
        navigation.order: 5
        onClicked: playbackModel.forwardStep()
    }

    TransportButton {
        icon: "loop.svg"
        tooltip: qsTr("Loop (Ctrl+L)")
        navigation.name: "Loop"
        navigation.panel: navPanel
        navigation.order: 6
        engaged: playbackModel.isLoopEnabled
        onClicked: playbackModel.toggleLoop()
    }
}
