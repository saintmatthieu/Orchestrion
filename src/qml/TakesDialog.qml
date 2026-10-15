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

// On closing a score while takes were recorded (Development > Record takes):
// which of the session's takes to keep, all of them to begin with. Returns
// the indices of those kept.
StyledDialogView {
    id: root

    // The takes' descriptions, in order.
    property var takes: []
    property var kept: []

    title: qsTrc("orchestrion/takes", "Keep the takes?")
    contentWidth: 420
    contentHeight: layout.implicitHeight + 2 * root.margins
    margins: 16

    Component.onCompleted: {
        var all = []
        for (var i = 0; i < root.takes.length; ++i) {
            all.push(i)
        }
        root.kept = all
    }

    function done(keep) {
        root.ret = { errcode: 0, value: keep ? root.kept : [] }
        root.hide()
    }

    ColumnLayout {
        id: layout

        anchors.fill: parent
        spacing: 12

        StyledTextLabel {
            Layout.fillWidth: true
            text: qsTrc("orchestrion/takes", "The takes played since the score was opened:")
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
        }

        Repeater {
            model: root.takes

            CheckBox {
                Layout.fillWidth: true
                text: modelData
                checked: root.kept.indexOf(index) >= 0
                onClicked: {
                    var kept = root.kept.slice()
                    var at = kept.indexOf(index)
                    if (at >= 0) {
                        kept.splice(at, 1)
                    } else {
                        kept.push(index)
                        kept.sort(function (a, b) { return a - b })
                    }
                    root.kept = kept
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            FlatButton {
                text: qsTrc("orchestrion/takes", "Discard all")
                onClicked: root.done(false)
            }

            FlatButton {
                text: qsTrc("orchestrion/takes", "Keep")
                accentButton: true
                enabled: root.kept.length > 0
                onClicked: root.done(true)
            }
        }
    }
}
