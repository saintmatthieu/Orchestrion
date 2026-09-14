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

// The window of a built-in effect (Effects > <effect> > Show…): bypass in the
// header, input and output level, gain reduction, clip indicator. The
// parameters themselves are edited in the JSON file named at the bottom.
StyledDialogView {
    id: root

    property string effect: "compressor"

    title: meters.title
    contentWidth: 380
    contentHeight: layout.implicitHeight + 2 * root.margins
    margins: 16
    modal: false

    EffectMeterModel {
        id: meters
        effect: root.effect
    }

    component MeterBar: Item {
        id: bar

        property string label: ""
        property double valueDb: -90
        property double minDb: -60
        property double maxDb: 0
        // Filled from the left (levels) or from the right (gain reduction).
        property bool fromRight: false
        property color fillColor: ui.theme.accentColor

        readonly property double fraction: Math.max(0, Math.min(1, (valueDb - minDb) / (maxDb - minDb)))

        Layout.fillWidth: true
        implicitHeight: 34

        StyledTextLabel {
            anchors.left: parent.left
            anchors.top: parent.top
            text: bar.label
            font: ui.theme.bodyFont
        }

        StyledTextLabel {
            anchors.right: parent.right
            anchors.top: parent.top
            text: bar.valueDb <= bar.minDb ? "–" : bar.valueDb.toFixed(1) + " dB"
            font: ui.theme.bodyFont
        }

        Rectangle {
            id: trough
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 10
            radius: 3
            color: ui.theme.buttonColor
            border.color: ui.theme.strokeColor
            border.width: 1

            Rectangle {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: bar.fromRight ? undefined : parent.left
                anchors.right: bar.fromRight ? parent.right : undefined
                width: trough.width * bar.fraction
                radius: 3
                color: bar.fillColor
            }
        }
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 10

        EffectHeader {
            Layout.fillWidth: true
            effectId: meters.effectId
        }

        MeterBar {
            label: qsTrc("effects", "Input")
            valueDb: meters.inputDb
        }

        MeterBar {
            label: qsTrc("effects", "Output")
            valueDb: meters.outputDb
        }

        MeterBar {
            visible: meters.hasGainReduction
            label: qsTrc("effects", "Gain reduction")
            // Shown growing from the right, as on a compressor's meter.
            valueDb: meters.gainReductionDb
            minDb: 0
            maxDb: 24
            fromRight: true
            fillColor: "#d9a441"
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: clipRow.implicitHeight

            Row {
                id: clipRow
                spacing: 8

                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    anchors.verticalCenter: parent.verticalCenter
                    color: meters.clipped ? "#d03030" : ui.theme.buttonColor
                    border.color: ui.theme.strokeColor
                    border.width: 1
                }

                StyledTextLabel {
                    text: meters.clipped ? qsTrc("effects", "Clipped (click to reset)") : qsTrc("effects", "No clipping")
                    font: ui.theme.bodyFont
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: meters.resetClip()
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            text: qsTrc("effects", "Parameters: %1 (saved changes apply at once)").arg(meters.parametersFilePath)
            font: ui.theme.bodyFont
            opacity: 0.7
            wrapMode: Text.WrapAnywhere
            horizontalAlignment: Text.AlignLeft
        }
    }
}
