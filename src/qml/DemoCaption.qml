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

import Orchestrion 1.0
import Orchestrion.OrchestrionShell 1.0

/**
 * A demo's caption (DemoKeys), in its pill, on as many lines as the width it is
 * given asks for. Whoever shows it decides where, and fades it.
 */
Rectangle {
    id: root

    /** The widest it may be. */
    property real maximumWidth: Infinity
    readonly property int horizontalPadding: 24

    width: caption.width + 2 * horizontalPadding
    height: caption.height + 20
    radius: Math.min(height / 2, 32)
    color: Theme.overlay
    border.color: Theme.metal
    border.width: 1

    Text {
        id: caption
        anchors.centerIn: parent
        width: Math.min(implicitWidth, root.maximumWidth - 2 * root.horizontalPadding)
        // Kept while fading out
        property string shown: DemoKeys.caption
        text: shown
        color: Theme.metalBright
        font.pixelSize: 28
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter

        Connections {
            target: DemoKeys
            function onCaptionChanged() {
                if (DemoKeys.caption.length > 0) {
                    caption.shown = DemoKeys.caption
                }
            }
        }
    }
}
