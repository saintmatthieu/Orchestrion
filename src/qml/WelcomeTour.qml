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
import Orchestrion.OrchestrionSequencer 1.0

// First-launch welcome tour. Instantiate with `anchors.fill: parent`; a scrim
// dims the notation view and a centered card walks through the essentials in
// a few pages. Shows until finished; "don't show this again" persists.
Item {
    id: root

    // Exposed so siblings (e.g. NumberKeysHelp) can hold back while the
    // tour is up.
    readonly property bool active: model.active

    property int page: 0
    readonly property int pageCount: 3

    visible: model.active

    // The rhythm page demoes on the score that is open behind the tour, and
    // the example-scores page drops the File menu open as a guide.
    onPageChanged: {
        if (page === 1)
            rhythmDemo.start()
        else
            rhythmDemo.stop()

        if (page === 2)
            model.openFileMenu()
        else
            model.closeAppMenu()
    }

    FirstRunWelcomeModel {
        id: model
        Component.onCompleted: model.init()
    }

    TourRhythmDemoModel {
        id: rhythmDemo
        Component.onCompleted: rhythmDemo.init()
    }

    component NavButton: Button {
        id: navButton
        property bool primary: false
        property bool compact: false
        font.pixelSize: compact ? 12 : 14
        font.bold: primary

        contentItem: Text {
            text: navButton.text
            font: navButton.font
            color: navButton.primary ? "#2B2B2B" : Theme.accent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            implicitWidth: navButton.compact ? 84 : 110
            implicitHeight: navButton.compact ? 26 : 34
            radius: 6
            color: navButton.primary
                   ? (navButton.down ? Qt.darker(Theme.accent, 1.2) : Theme.accent)
                   : (navButton.down ? Qt.rgba(1, 1, 1, 0.1) : "transparent")
            border.color: Theme.accent
            border.width: navButton.primary ? 0 : 1
        }
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
        height: column.implicitHeight + 36
        radius: 12
        color: "#2B2B2B"
        border.color: Theme.accent
        border.width: 1

        Column {
            id: column
            anchors.centerIn: parent
            spacing: 14

            // Fixed-size stage so the card does not resize between pages.
            Item {
                id: stage
                width: 480
                height: 190

                // ---- Page 0: welcome -------------------------------------
                Column {
                    visible: root.page === 0
                    anchors.centerIn: parent
                    spacing: 12
                    width: parent.width

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: "qrc:/icons/orchestrion.png"
                        sourceSize.width: 56
                        sourceSize.height: 56
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Welcome to Orchestrion")
                        color: Theme.accent
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Text {
                        width: parent.width * 0.9
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("You tap the rhythm, Orchestrion plays the notes.")
                        color: "#CCCCCC"
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // ---- Page 1: tap the rhythm ------------------------------
                Column {
                    visible: root.page === 1
                    anchors.centerIn: parent
                    spacing: 8
                    width: parent.width

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Tap the rhythm")
                        color: Theme.accent
                        font.pixelSize: 18
                        font.bold: true
                    }

                    // The shared number-keys animation, following the score
                    // auto-playing behind the tour.
                    Item {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: keysDemo.implicitWidth * keysDemo.scale
                        height: keysDemo.implicitHeight * keysDemo.scale

                        NumberKeysAnimation {
                            id: keysDemo
                            scale: 0.66
                            transformOrigin: Item.TopLeft
                            leftPressedKey: rhythmDemo.leftPressedKey
                            rightPressedKey: rhythmDemo.rightPressedKey
                        }
                    }

                    Text {
                        width: parent.width * 0.95
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Press keys to the rhythm of the piece — Orchestrion plays the right notes. Left hand: keys 1 to 5, right hand: keys 6 to 0.")
                        color: "#CCCCCC"
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }

                    NavButton {
                        anchors.horizontalCenter: parent.horizontalCenter
                        compact: true
                        text: rhythmDemo.muted ? qsTr("Unmute") : qsTr("Mute")
                        onClicked: rhythmDemo.toggleMuted()
                    }
                }

                // ---- Page 2: bring your own music ------------------------
                Column {
                    visible: root.page === 2
                    anchors.centerIn: parent
                    spacing: 12
                    width: parent.width

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Bring your own music")
                        color: Theme.accent
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Text {
                        width: parent.width * 0.9
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Start with the example scores, or open any MuseScore or MusicXML file from the File menu, whenever you are ready.")
                        color: "#CCCCCC"
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }

                    CheckBox {
                        id: dontShowAgain
                        anchors.horizontalCenter: parent.horizontalCenter
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

            // ---- Page dots -----------------------------------------------
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8
                Repeater {
                    model: root.pageCount
                    delegate: Rectangle {
                        required property int index
                        width: 8
                        height: 8
                        radius: 4
                        color: index === root.page ? Theme.accent : "transparent"
                        border.color: Theme.accent
                        border.width: 1
                    }
                }
            }

            // ---- Navigation ----------------------------------------------
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 18

                NavButton {
                    text: qsTr("Back")
                    opacity: root.page > 0 ? 1 : 0
                    enabled: root.page > 0
                    onClicked: root.page = root.page - 1
                }

                NavButton {
                    primary: true
                    text: root.page < root.pageCount - 1 ? qsTr("Next")
                                                         : qsTr("Let's play!")
                    onClicked: {
                        if (root.page < root.pageCount - 1) {
                            root.page = root.page + 1
                        } else {
                            rhythmDemo.stop()
                            model.closeAppMenu()
                            model.dismiss(dontShowAgain.checked)
                        }
                    }
                }
            }
        }
    }
}
