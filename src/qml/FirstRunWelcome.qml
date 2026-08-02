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

import Orchestrion 1.0
import Orchestrion.OrchestrionOnboarding 1.0

// First-launch welcome card. Instantiate with `anchors.fill: parent`; a scrim
// dims the notation view and a centered card explains the three things a new
// user needs to know. Shows until dismissed; "don't show this again" persists.
Item {
    id: root

    // Exposed so siblings (e.g. NumberKeysHelp) can hold back while the
    // welcome is up.
    readonly property bool active: model.active

    visible: model.active

    FirstRunWelcomeModel {
        id: model
        Component.onCompleted: model.init()
    }

    Rectangle {
        id: scrim
        anchors.fill: parent
        color: "#66000000"

        // Swallow clicks so the notation view underneath is not disturbed.
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
        }
    }

    Rectangle {
        id: card

        anchors.centerIn: parent
        width: column.implicitWidth + 48
        height: column.implicitHeight + 40
        radius: 12
        color: "#2B2B2B"
        border.color: Theme.accent
        border.width: 1

        Column {
            id: column
            anchors.centerIn: parent
            spacing: 14

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/icons/orchestrion.png"
                    sourceSize.width: 36
                    sourceSize.height: 36
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Welcome to Orchestrion")
                    color: Theme.accent
                    font.pixelSize: 20
                    font.bold: true
                }
            }

            Repeater {
                model: [
                    { title: qsTr("Choose how you play"),
                      text: qsTr("Plug in a MIDI keyboard, or just use the computer keyboard — switch controllers in the Advanced menu.") },
                    { title: qsTr("Tap the rhythm"),
                      text: qsTr("Press keys to the rhythm of the piece — Orchestrion plays the right notes.") },
                    { title: qsTr("Bring your own music"),
                      text: qsTr("Open any MuseScore or MusicXML file from the File menu, whenever you are ready.") }
                ]
                delegate: Row {
                    required property var modelData
                    required property int index
                    spacing: 12

                    Rectangle {
                        width: 26
                        height: 26
                        radius: 13
                        color: "transparent"
                        border.color: Theme.accent
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: index + 1
                            color: Theme.accent
                            font.pixelSize: 14
                            font.bold: true
                        }
                    }

                    Column {
                        width: 340
                        spacing: 2

                        Text {
                            text: modelData.title
                            color: "white"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text {
                            width: parent.width
                            text: modelData.text
                            color: "#CCCCCC"
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 18

                Button {
                    id: playButton
                    text: qsTr("Let's play!")
                    font.pixelSize: 14
                    font.bold: true

                    contentItem: Text {
                        text: playButton.text
                        font: playButton.font
                        color: "#2B2B2B"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitWidth: 120
                        implicitHeight: 36
                        radius: 6
                        color: playButton.down ? Qt.darker(Theme.accent, 1.2)
                                               : Theme.accent
                    }

                    onClicked: model.dismiss(dontShowAgain.checked)
                }

                CheckBox {
                    id: dontShowAgain
                    anchors.verticalCenter: parent.verticalCenter
                    checked: true
                    text: qsTr("Don't show this again")

                    contentItem: Text {
                        text: dontShowAgain.text
                        color: "#CCCCCC"
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: dontShowAgain.indicator.width + dontShowAgain.spacing
                    }
                }
            }
        }
    }
}
