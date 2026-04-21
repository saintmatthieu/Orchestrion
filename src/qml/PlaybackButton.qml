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
import Muse.UiComponents 1.0
import Orchestrion.OrchestrionShell 1.0

Row {
    id: root

    spacing: 2

    PlaybackButtonModel {
        id: playbackModel
    }

    Component.onCompleted: {
        playbackModel.load()
    }

    Repeater {
        model: [
            { icon: "back", action: () => playbackModel.rewind(), tooltip: "Rewind (Home)" },
            { icon: "rewind-button", action: () => playbackModel.backStep(), tooltip: "Previous (Left)" },
            { icon: playbackModel.isPlaying ? "pause-button" : "play", action: () => playbackModel.togglePlay(), tooltip: "Play/Pause (Space)" },
            { icon: "stop-sign", action: () => playbackModel.stop(), tooltip: "Stop" },
            { icon: "rewind-button", action: () => playbackModel.forwardStep(), tooltip: "Next (Right)", flipped: true }
        ]

        Image {
            id: iconImage
            source: "qrc:/icons/player/" + modelData.icon + ".png"
            width: 36
            height: 36
            fillMode: Image.PreserveAspectFit
            opacity: 0.7
            mipmap: true
            rotation: modelData.flipped ? 180 : 0

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onEntered: { hideTimer.stop(); tip.visible = true }
                onExited: hideTimer.restart()
                onClicked: modelData.action()
            }

            Timer {
                id: hideTimer
                interval: 300
                onTriggered: tip.visible = false
            }

            ToolTip {
                id: tip
                delay: 500
                contentItem: Text {
                    text: modelData.tooltip + "<br><span style='font-size:9px'><a href='https://www.flaticon.com/free-icons/ui' title='ui icons'>Ui icons created by chehuna - Flaticon</a></span>"
                    textFormat: Text.RichText
                    onLinkActivated: function(link) {
                        Qt.openUrlExternally(link);
                    }
                    HoverHandler {
                        cursorShape: Qt.PointingHandCursor
                        onHoveredChanged: {
                            if (hovered) {
                                hideTimer.stop()
                            } else {
                                hideTimer.restart()
                            }
                        }
                    }
                }
            }
        }
    }
}
