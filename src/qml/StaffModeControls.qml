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
import QtQuick 2.15

import Muse.Ui as MUI // qualified, like Main.qml's

import Orchestrion 1.0

/**
 * The staves' mode arrows as controls, for the keyboard and screen readers.
 * The score view paints them, left of its left cylinder, and says where
 * (OrchestrionNotationPaintView.staffModeControls): these sit over them,
 * invisible, and leave the mouse to the view, which also draws the focus, on
 * the arrow itself. One of them focused keeps the cylinder rolled in.
 */
Item {
    id: root

    /** The OrchestrionNotationPaintView. */
    property var view
    property MUI.NavigationSection navigationSection

    MUI.NavigationPanel {
        id: navPanel
        name: "StaffModes"
        section: root.navigationSection
        // Before the transport's: they are left of it
        order: 0
        accessible.name: qsTr("Staff modes")
    }

    // Worked out from all of them at once: each control's highlight changes
    // whenever the focus is shown or not, anywhere, so following their
    // signals one by one gets the order wrong.
    // Whether one has the focus, from the keyboard or not: that uncovers the
    // modes.
    readonly property bool anyFocused: {
        for (let i = 0; i < arrows.count; ++i) {
            const arrow = arrows.itemAt(i)
            if (arrow && arrow.focused) {
                return true
            }
        }
        return false
    }
    // The one showing the focus, if any, which the view draws larger and in
    // metal: shown only when the focus came from the keyboard.
    readonly property var highlightedArrow: {
        for (let i = 0; i < arrows.count; ++i) {
            const arrow = arrows.itemAt(i)
            if (arrow && arrow.highlighted) {
                return arrow.modelData
            }
        }
        return null
    }
    onAnyFocusedChanged: root.view.holdModesUncovered(anyFocused)
    onHighlightedArrowChanged: root.view.focusStaffModeArrow(
                                   highlightedArrow ? highlightedArrow.staff : -1,
                                   highlightedArrow ? highlightedArrow.next : false)

    Repeater {
        id: arrows
        // Fixed, so that a control keeps the focus while its arrow moves or
        // its staff's mode changes
        model: [
            { staff: 0, next: false, name: "UpperPrevious" },
            { staff: 0, next: true, name: "UpperNext" },
            { staff: 1, next: false, name: "LowerPrevious" },
            { staff: 1, next: true, name: "LowerNext" }
        ]

        Item {
            id: arrow

            required property var modelData

            // This staff's, if it is played
            readonly property var staffControls: {
                const controls = root.view.staffModeControls
                for (let i = 0; i < controls.length; ++i) {
                    if (controls[i].staff === modelData.staff) {
                        return controls[i]
                    }
                }
                return null
            }
            readonly property rect area: staffControls
                                         ? (modelData.next ? staffControls.next : staffControls.previous)
                                         : Qt.rect(0, 0, 0, 0)

            x: area.x
            y: area.y
            width: area.width
            height: area.height
            visible: staffControls !== null
            readonly property bool focused: navCtrl.active
            readonly property bool highlighted: navCtrl.highlight

            MUI.NavigationControl {
                id: navCtrl
                name: arrow.modelData.name
                panel: navPanel
                order: arrow.modelData.staff * 2 + (arrow.modelData.next ? 2 : 1)
                enabled: arrow.visible

                accessible.role: MUI.MUAccessible.Button
                accessible.name: (arrow.modelData.staff === 0 ? qsTr("Upper staff")
                                                              : qsTr("Lower staff"))
                                 + ": " + (arrow.staffControls ? arrow.staffControls.mode : "")
                                 + ". " + (arrow.modelData.next ? qsTr("Next mode")
                                                                : qsTr("Previous mode"))
                accessible.visualItem: arrow
                accessible.enabled: navCtrl.enabled

                onTriggered: root.view.stepStaffMode(arrow.modelData.staff,
                                                     arrow.modelData.next)
            }
        }
    }
}
