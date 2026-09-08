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
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

import Orchestrion.OrchestrionShell 1.0

// The sustain-pedal indicator, centred at the bottom of the score view,
// opposite the grading toggle: a piano's three pedals seen head-on from the
// floor, brass over the lyre's dark slots. The rightmost pedal is the
// sustain pedal: it slides down its slot while the sequencer holds the
// pedal down and back up when it lifts. The position alone tells the state,
// so nothing is dimmed. An indicator only: the pedalling follows the score's
// markings, so there is nothing to click.
Item {
    id: root

    // The drawings share a 27.33 x 4.3 unit canvas, drawn at 4 px per unit;
    // the pedal's travel is the slot's height minus the pedal's, 1.1 units.
    readonly property real unit: 4
    readonly property int iconWidth: Math.round(27.33 * unit)
    readonly property int iconHeight: Math.round(4.3 * unit)
    readonly property real travel: 1.1 * unit

    width: iconWidth + 8
    height: 36

    readonly property bool pedalDown: model.pedalDown

    PedalIndicatorModel {
        id: model
        Component.onCompleted: load()
    }

    // The icons carry their own colours (gold pedals, cream-rimmed slots), so
    // they are drawn as they are, without the colour overlay the monochrome
    // top-row icons go through. Rasterised at the displayed size, for a
    // sharp result at any pixel ratio.
    Image {
        id: base
        anchors.centerIn: parent
        width: root.iconWidth
        height: root.iconHeight
        sourceSize.width: width * Screen.devicePixelRatio
        sourceSize.height: height * Screen.devicePixelRatio
        source: "qrc:/icons/player/pedals.svg"
        fillMode: Image.PreserveAspectFit
    }

    // The sustain pedal's bar, drawn at its up position on the same canvas
    // and slid down over its slot.
    Image {
        anchors.horizontalCenter: base.horizontalCenter
        y: base.y + (root.pedalDown ? root.travel : 0)
        Behavior on y {
            NumberAnimation { duration: 120; easing.type: Easing.OutQuad }
        }
        width: root.iconWidth
        height: root.iconHeight
        sourceSize.width: width * Screen.devicePixelRatio
        sourceSize.height: height * Screen.devicePixelRatio
        source: "qrc:/icons/player/pedal-sustain.svg"
        fillMode: Image.PreserveAspectFit
    }

    HoverHandler {
        id: hover
    }

    ToolTip {
        delay: 500
        visible: hover.hovered
        text: root.pedalDown ? qsTr("Sustain pedal (down)")
                             : qsTr("Sustain pedal (up)")
    }
}
