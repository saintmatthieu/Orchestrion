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
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// The grading configuration dialog: the master switch and all its dependent
// settings. Opened from the Grading menu's "Settings…" item.
Popup {
    id: root

    property var model

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 24

    background: Rectangle {
        color: "#F5241811"
        border.color: "#E5B84B"
        border.width: 2
        radius: 12
    }

    readonly property color ink: "#F0E5C8"
    readonly property color dimInk: "#C9B583"

    // User interaction breaks declarative `checked` bindings; the master
    // switch is also driven from outside (top-row button, Grading menu), so
    // resync it explicitly.
    Connections {
        target: root.model
        function onGradingEnabledChanged() {
            masterBox.checked = root.model.gradingEnabled
        }
    }

    contentItem: ColumnLayout {
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Grading")
                color: root.ink
                font.pixelSize: 14
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "✕"
                color: root.dimInk
                font.pixelSize: 16

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        SettingBox {
            id: masterBox
            enabled: true // the master switch itself never grays out
            text: qsTr("Grading enabled")
            checked: root.model ? root.model.gradingEnabled : false
            onToggled: root.model.gradingEnabled = checked
        }

        component SettingBox: CheckBox {
            id: box
            // `text` must carry the label even with a custom contentItem:
            // the style centres the indicator when it is empty.
            enabled: root.model && root.model.gradingEnabled
            contentItem: Text {
                text: box.text
                color: box.enabled ? root.ink : root.dimInk
                font.pixelSize: 14
                leftPadding: box.indicator.width + 8
                verticalAlignment: Text.AlignVCenter
            }
        }

        SettingBox {
            text: qsTr("Keep timing marks on screen")
            checked: root.model ? root.model.persistentMarks : false
            onToggled: root.model.persistentMarks = checked
        }

        SettingBox {
            text: qsTr("Hand-synchronization sub-score")
            checked: root.model ? root.model.handSyncScore : false
            onToggled: root.model.handSyncScore = checked
        }

        SettingBox {
            text: qsTr("Dynamics sub-score")
            checked: root.model ? root.model.dynamicsScore : false
            onToggled: root.model.dynamicsScore = checked
        }

        SettingBox {
            text: qsTr("Time-proportional spacing")
            checked: root.model ? root.model.proportionalSpacing : false
            onToggled: root.model.proportionalSpacing = checked
        }

        Text {
            text: qsTr("Play button")
            color: root.model && root.model.gradingEnabled ? root.ink
                                                           : root.dimInk
            font.pixelSize: 14
            font.bold: true
            topPadding: 8
        }

        component ModeButton: RadioButton {
            id: radio
            property int mode
            enabled: root.model && root.model.gradingEnabled
            checked: root.model && root.model.playMode === mode
            onClicked: root.model.playMode = mode
            contentItem: Text {
                text: radio.text
                color: radio.enabled ? root.ink : root.dimInk
                font.pixelSize: 14
                leftPadding: radio.indicator.width + 8
                verticalAlignment: Text.AlignVCenter
            }
        }

        ModeButton { text: qsTr("Replay performance"); mode: 0 }
        ModeButton { text: qsTr("Replay at fitted tempo"); mode: 1 }
        ModeButton { text: qsTr("Metronomic playback"); mode: 2 }
    }
}
