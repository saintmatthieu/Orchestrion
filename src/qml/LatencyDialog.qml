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
import Muse.Ui
import Muse.UiComponents
import Orchestrion.OrchestrionSynthesis 1.0
import Orchestrion 1.0

// Audio/MIDI > Latency…: the margin the audio driver keeps against
// underruns (automatic, or chosen), and what it measures: the latency, the
// headroom, the load, and the underruns of the last two minutes.
StyledDialogView {
    id: root

    title: qsTrc("orchestrion/latency", "Latency")
    contentWidth: 460
    contentHeight: layout.implicitHeight + 2 * root.margins
    margins: 16
    modal: false

    OutputLatencyModel {
        id: model
    }

    readonly property var s: model.status
    readonly property bool available: s.available === true
    readonly property color underrunColor: "#d03030"
    readonly property color stallColor: "#d09030"

    function ms(value) {
        return value.toFixed(1) + " ms"
    }

    component Caption: StyledTextLabel {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignLeft
        font: ui.theme.bodyBoldFont
    }

    component Line: StyledTextLabel {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        font: ui.theme.bodyFont
    }

    ColumnLayout {
        id: layout

        anchors.fill: parent
        spacing: 10

        Line {
            text: root.available
                  ? qsTrc("orchestrion/latency", "%1, the device's period: %2").arg(root.s.driver).arg(root.ms(root.s.periodMs))
                  : qsTrc("orchestrion/latency", "Not available with this audio driver.")
        }

        // The margin the driver keeps, a level per whole period of the device
        Caption {
            visible: root.available
            text: qsTrc("orchestrion/latency", "Latency")
        }

        Line {
            visible: root.available
            text: qsTrc("orchestrion/latency", "Lower latency makes the playing experience snappier but with a higher risk of audio glitches.")
        }

        RowLayout {
            visible: root.available
            spacing: 4

            CheckBox {
                text: qsTrc("orchestrion/latency", "Automatic")
                checked: root.s.automatic === true
                onClicked: model.setAutomatic(!checked)
            }

            // The ringed ? of the main window's tutorials button
            TransportButton {
                icon: "help.svg"
                width: 26
                height: 26
                onClicked: helpPopup.toggleOpened()

                StyledPopupView {
                    id: helpPopup

                    contentWidth: 280
                    contentHeight: helpText.implicitHeight

                    StyledTextLabel {
                        id: helpText

                        width: 280
                        text: qsTrc("orchestrion/latency", "With lots of headroom for a long time, will lower the latency. If getting dangerously close to underruns, will increase it. Only takes effect when the app is not playing sound.")
                        horizontalAlignment: Text.AlignLeft
                        wrapMode: Text.WordWrap
                        font: ui.theme.bodyFont
                    }
                }
            }
        }

        Row {
            visible: root.available
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: root.available ? root.s.levelsMarginMs.length : 0

                FlatRadioButton {
                    required property int index

                    width: (layout.width - (root.s.levelsMarginMs.length - 1) * 6) / root.s.levelsMarginMs.length
                    text: [qsTrc("orchestrion/latency", "Low"), qsTrc("orchestrion/latency", "Medium"),
                           qsTrc("orchestrion/latency", "Large")][index]
                    checked: root.s.level === index
                    // Still a choice while automatic: making one switches it off
                    opacity: root.s.automatic === true ? 0.5 : 1.0
                    onToggled: model.setLevel(index)
                }
            }
        }

        // What is measured
        Caption {
            visible: root.available
            text: qsTrc("orchestrion/latency", "Measured")
        }

        Line {
            visible: root.available
            text: qsTrc("orchestrion/latency", "Headroom (last 10 s): %1 on average, %2 at least")
                  .arg(root.ms(root.s.headroomMeanMs)).arg(root.ms(root.s.headroomMinMs))
        }

        Line {
            visible: root.available
            text: qsTrc("orchestrion/latency", "Load (last 10 s): %1 % on average, %2 % at most")
                  .arg(root.s.loadMeanPercent.toFixed(1)).arg(root.s.loadMaxPercent.toFixed(1))
        }

        // The underruns
        Caption {
            visible: root.available
            text: qsTrc("orchestrion/latency", "Underruns")
        }

        Line {
            visible: root.available
            text: {
                if (root.s.underruns === 0) {
                    return qsTrc("orchestrion/latency", "None")
                }
                var since = Math.round(root.s.secondsSinceLastUnderrun)
                return qsTrc("orchestrion/latency", "%1, the last %2 s ago").arg(root.s.underruns).arg(since)
            }
            color: root.s.underruns > 0 ? root.underrunColor : ui.theme.fontPrimaryColor
        }

        Line {
            visible: root.available
            text: qsTrc("orchestrion/latency", "Silence played by the device: %1 (Orchestrion's underruns, and the system's)")
                  .arg(root.s.clockStalls)
            opacity: 0.7
        }

        // The last two minutes, a second per cell, now at the right
        Item {
            visible: root.available
            Layout.fillWidth: true
            implicitHeight: 32

            Row {
                id: strip

                anchors.left: parent.left
                anchors.right: parent.right
                height: 18
                spacing: 1

                Repeater {
                    model: root.available ? root.s.underrunsPerSecond.length : 0

                    Rectangle {
                        required property int index

                        readonly property int underruns: root.s.underrunsPerSecond[index]
                        readonly property int stalls: root.s.clockStallsPerSecond[index]

                        width: (strip.width - (root.s.underrunsPerSecond.length - 1) * strip.spacing) / root.s.underrunsPerSecond.length
                        height: strip.height
                        color: underruns > 0 ? root.underrunColor : stalls > 0 ? root.stallColor : ui.theme.buttonColor
                    }
                }
            }

            StyledTextLabel {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                text: qsTrc("orchestrion/latency", "2 min ago")
                font: ui.theme.bodyFont
                opacity: 0.6
            }

            StyledTextLabel {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                text: qsTrc("orchestrion/latency", "now")
                font: ui.theme.bodyFont
                opacity: 0.6
            }
        }

        RowLayout {
            visible: root.available
            Layout.fillWidth: true
            spacing: 8

            FlatButton {
                text: qsTrc("orchestrion/latency", "Start over")
                onClicked: model.resetCounters()
            }

            Item {
                Layout.fillWidth: true
            }

            // Late by more than a period: an underrun at the least margin only
            FlatButton {
                text: qsTrc("orchestrion/latency", "Test: a callback 15 ms late")
                onClicked: model.delayNextCallback(15)
            }
        }
    }
}
