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
import Muse.Ui
import Muse.UiComponents
import Muse.Vst
import Orchestrion 1.0

// A VST effect's editor window: the framework's editor item under
// Orchestrion's effect header (bypass). Registered for muse://vst/editor in
// place of MuseScore's VstEditorDialog.
StyledDialogView {
    id: root

    property alias instanceId: view.instanceId
    // Passed by the VST module's open-editor action; a floating window is
    // what a dialog view is anyway.
    property bool floating: true

    title: view.title
    contentWidth: view.implicitWidth
    contentHeight: header.height + view.implicitHeight
    alwaysAboveApp: true

    Rectangle {
        anchors.fill: parent
        color: ui.theme.backgroundPrimaryColor
    }

    EffectHeader {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        vstInstanceId: root.instanceId
    }

    // The plugin's own window is a native child window placed by the view at
    // (sidePadding, topPadding) of the dialog window, whatever the item's
    // position: the header's height goes in as the top padding.
    VstView {
        id: view
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        topPadding: header.height

        Component.onCompleted: {
            view.init()
        }
    }
}
