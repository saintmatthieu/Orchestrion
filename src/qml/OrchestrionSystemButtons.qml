/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * Adapted from MuseScore's appshell QML (GPL-3.0-only,
 * Copyright (C) 2021 MuseScore BVBA and others).
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
import Muse.Ui
import Muse.UiComponents
import Orchestrion 1.0

// The minimize / maximize / close buttons of the in-window title bar
// (Windows and Linux). MuseScore only ships its own on Windows.
Row {
    id: root

    property bool windowIsMiximized: false

    spacing: 8

    signal showWindowMinimizedRequested()
    signal toggleWindowMaximizedRequested()
    signal closeWindowRequested()

    FlatButton {
        id: minimizeButton
        icon: IconCode.APP_MINIMIZE
        iconColor: Theme.accent
        transparent: true
        drawFocusBorderInsideRect: true
        backgroundItem: TitleBarButtonBackground {
            mouseArea: minimizeButton.mouseArea
        }
        onClicked: {
            root.showWindowMinimizedRequested()
        }
    }

    FlatButton {
        id: maximizeButton
        icon: !root.windowIsMiximized ? IconCode.APP_MAXIMIZE : IconCode.APP_UNMAXIMIZE
        iconColor: Theme.accent
        transparent: true
        drawFocusBorderInsideRect: true
        backgroundItem: TitleBarButtonBackground {
            mouseArea: maximizeButton.mouseArea
        }
        onClicked: {
            root.toggleWindowMaximizedRequested()
        }
    }

    FlatButton {
        id: closeButton
        icon: IconCode.APP_CLOSE
        iconColor: Theme.accent
        transparent: true
        drawFocusBorderInsideRect: true
        backgroundItem: TitleBarButtonBackground {
            mouseArea: closeButton.mouseArea
        }
        onClicked: {
            root.closeWindowRequested()
        }
    }
}
