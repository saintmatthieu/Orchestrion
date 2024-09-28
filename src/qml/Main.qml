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
import MuseScore.NotationScene 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 400
    height: 300
    title: qsTr("Hello, World!")

    Component.onCompleted: {
        console.log("Hello, World!")
    }

    NotationView {
        id: notationView
        name: "MainNotationView"

        isNavigatorVisible: false
        isBraillePanelVisible: false
        isMainView: true
    }
}
