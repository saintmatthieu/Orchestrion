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

import Orchestrion 1.0

// The metal lining of the score's backdrop: a thin centred rule broken by
// a small diamond, mirrored at the top and bottom of the view. Given a title,
// the rule opens up and carries it between two diamonds, with an optional
// subtitle (the composer) in smaller capitals beneath:
//
//     ————————————  ◆  Für Elise  ◆  ————————————
//                  Ludwig van Beethoven
//
// Drawn live rather than baked into the wallpaper so that the rules stay
// crisp at any window size and the title can sit in the line.
Item {
    id: root

    property string title: ""
    // Shown centred under the title, smaller. Ignored without a title.
    property string subtitle: ""
    // Whether the title (with its subtitle) is a button: it then lights up
    // under the mouse and emits titleClicked().
    property bool titleClickable: false
    // Shows the chevron under the title that says it opens something, as the
    // other controls do on mouse activity (it also shows on hover).
    property bool titleHintShown: false

    signal titleClicked()

    readonly property bool titleHovered: titleMouse.containsMouse

    // The view the ornament decorates; the rules span a fixed fraction of its
    // width and the title scales with its height.
    property real viewWidth: parent ? parent.width : 800
    property real viewHeight: parent ? parent.height : 600

    readonly property color metal: root.titleHovered ? Theme.metalBright
                                                     : Theme.metal
    readonly property real gap: 12
    readonly property real diamondHalf: 5
    // Each rule, so that the plain ornament spans 32 % of the view.
    readonly property real ruleLength: Math.max(
        viewWidth * 0.16 - gap - diamondHalf, 40)
    readonly property bool hasTitle: root.title.length > 0
    readonly property bool hasSubtitle: hasTitle && root.subtitle.length > 0
    // Kept out of the Text's grouped `font` so that letterSpacing may depend
    // on it without a binding loop.
    readonly property int titlePixelSize: Math.round(
        Math.min(Math.max(viewHeight * 0.036, 14), 44))

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    // The item's own height stops at the rule, because the subtitle hangs
    // outside it (see below). These say where the ink really ends, and how
    // much air the layout puts between the title and the composer — enough
    // for a caller to keep the same gap under it.
    readonly property real inkBottom: subtitleText.visible
        ? subtitleText.y + subtitleText.height
        : row.y + row.height
    readonly property real titleGap: subtitleText.visible
        ? subtitleText.anchors.topMargin
        : 0

    FontLoader {
        id: titleFont
        source: "qrc:/fonts/CinzelDecorative/CinzelDecorative-Regular.ttf"
    }

    // A solid hairline with a soft halo, for a faint glow on the backdrop.
    component Rule: Item {
        width: root.ruleLength
        height: 4

        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: 4
            color: root.metal
            opacity: 0.28
            antialiasing: true
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: 1.5
            color: root.metal
            antialiasing: true
        }
    }

    component Diamond: Item {
        width: 2 * root.diamondHalf
        height: width

        Rectangle {
            anchors.centerIn: parent
            width: parent.width / Math.SQRT2
            height: width
            rotation: 45
            color: root.metal
            antialiasing: true
        }
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: root.gap

        Rule { anchors.verticalCenter: parent.verticalCenter }

        Diamond { anchors.verticalCenter: parent.verticalCenter }

        Text {
            id: titleText
            visible: root.hasTitle
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            color: root.metal
            font.family: titleFont.name
            font.pixelSize: root.titlePixelSize
            font.letterSpacing: root.titlePixelSize * 0.06
            maximumLineCount: 1
            // Long titles shrink a little, then get elided, rather than
            // pushing the rules off-screen.
            width: Math.min(implicitWidth, root.viewWidth * 0.5)
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: Math.round(root.titlePixelSize * 0.7)
            elide: Text.ElideRight
            // The box keeps the full-size height; keep a shrunk title on the
            // rule.
            verticalAlignment: Text.AlignVCenter
            renderType: Text.NativeRendering
        }

        Diamond {
            visible: root.hasTitle
            anchors.verticalCenter: parent.verticalCenter
        }

        Rule { anchors.verticalCenter: parent.verticalCenter }
    }

    // Hangs below the rule (outside the item's own bounds, so the rule stays
    // where the caller put it, subtitle or not).
    Text {
        id: subtitleText
        visible: root.hasSubtitle
        anchors.top: row.bottom
        anchors.topMargin: Math.round(root.titlePixelSize * 0.35)
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.subtitle
        color: root.metal
        opacity: 0.85
        font.family: titleFont.name
        font.pixelSize: Math.max(11, Math.round(root.titlePixelSize * 0.5))
        font.letterSpacing: root.titlePixelSize * 0.07
        maximumLineCount: 1
        width: Math.min(implicitWidth, root.viewWidth * 0.5)
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        renderType: Text.NativeRendering
    }

    // Under the title (or its subtitle): what clicking it does.
    Text {
        id: chevron
        anchors.top: root.hasSubtitle ? subtitleText.bottom : row.bottom
        anchors.topMargin: Math.round(root.titlePixelSize * 0.15)
        anchors.horizontalCenter: parent.horizontalCenter
        visible: opacity > 0
        opacity: root.titleClickable && root.hasTitle
                 && (root.titleHintShown || root.titleHovered) ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 250 } }
        text: "\u25BE"
        color: root.metal
        font.pixelSize: Math.max(11, Math.round(root.titlePixelSize * 0.5))
    }

    // The title, its diamonds and the subtitle.
    MouseArea {
        id: titleMouse
        enabled: root.titleClickable && root.hasTitle
        x: row.x + titleText.x - root.gap - 2 * root.diamondHalf
        y: row.y
        width: titleText.width + 2 * (root.gap + 2 * root.diamondHalf)
        height: (chevron.visible ? chevron.y + chevron.height : root.inkBottom)
                - row.y
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.titleClicked()
    }
}
