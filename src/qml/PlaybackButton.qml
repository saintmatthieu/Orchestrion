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

import Muse.Ui 1.0
import Muse.UiComponents 1.0
import Orchestrion.OrchestrionShell 1.0

RowLayout {
    id: root

    spacing: 2

    PlaybackButtonModel {
        id: playbackModel
    }

    Component.onCompleted: {
        playbackModel.load()
    }

    FlatButton {
        icon: IconCode.REWIND
        iconFont: ui.theme.toolbarIconsFont
        toolTipTitle: qsTr("Rewind to start")
        transparent: true
        enabled: playbackModel.isPlayAllowed
        onClicked: playbackModel.rewind()
    }

    FlatButton {
        icon: IconCode.SMALL_ARROW_LEFT
        iconFont: ui.theme.toolbarIconsFont
        toolTipTitle: qsTr("Step back")
        transparent: true
        enabled: playbackModel.isPlayAllowed
        onClicked: playbackModel.backStep()
    }

    FlatButton {
        icon: playbackModel.isPlaying ? IconCode.PAUSE : IconCode.PLAY
        iconFont: ui.theme.toolbarIconsFont
        toolTipTitle: playbackModel.isPlaying ? qsTr("Pause") : qsTr("Play")
        transparent: true
        enabled: playbackModel.isPlayAllowed
        onClicked: playbackModel.togglePlay()
    }

    FlatButton {
        icon: IconCode.STOP
        iconFont: ui.theme.toolbarIconsFont
        toolTipTitle: qsTr("Stop")
        transparent: true
        enabled: playbackModel.isPlayAllowed
        onClicked: playbackModel.stop()
    }

    FlatButton {
        icon: IconCode.SMALL_ARROW_RIGHT
        iconFont: ui.theme.toolbarIconsFont
        toolTipTitle: qsTr("Step forward")
        transparent: true
        enabled: playbackModel.isPlayAllowed
        onClicked: playbackModel.forwardStep()
    }
}
