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

// Background of the in-window title bar's buttons (menu titles and system
// buttons): a cream tint over the mahogany bar, shown on hover, on press and
// while the button's menu is open. Stands in for MuseScore's
// AppButtonBackground, whose theme colors (grey hover, accent blue for an
// open menu) don't sit well on the mahogany.
Rectangle {
    id: root

    required property MouseArea mouseArea

    // E.g. the menu this button opens is currently shown.
    property bool active: false

    color: Theme.accent
    opacity: root.mouseArea.pressed ? 0.3
           : root.active ? 0.25
           : root.mouseArea.containsMouse ? 0.15
           : 0
}
