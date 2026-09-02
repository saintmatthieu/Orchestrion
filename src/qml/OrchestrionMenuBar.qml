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
import MuseScore.AppShell
import Orchestrion.MuseScoreShell 1.0

// The in-window menu bar (Windows and Linux), showing Orchestrion's menus
// (OrchestrionMenuModel). The macOS top ribbon is MacMenuBar.qml.
Item {
    id: root

    property alias appWindow: appMenuModel.appWindow
    property int availableWidth: ui.rootItem.width
    property bool truncated: availableWidth < contentRow.childrenRect.width

    implicitWidth: contentRow.width
    implicitHeight: contentRow.height

    OrchestrionMenuModel {
        id: appMenuModel

        appMenuAreaRect: Qt.rect(root.x, root.y, root.width, root.height)
        openedMenuAreaRect: prv.openedArea(menuLoader)

        onOpenMenuRequested: function(menuId, byHover) {
            prv.openMenu(menuId, byHover)
        }

        onCloseOpenedMenuRequested: {
            menuLoader.close()
        }
    }

    AccessibleItem {
        id: panelAccessibleInfo
        visualItem: root
        role: MUAccessible.Panel
        name: qsTrc("appshell", "Application menu")
    }

    Component.onCompleted: {
        appMenuModel.load()
    }

    Row {
        id: contentRow

        Repeater {
            model: appMenuModel

            delegate: FlatButton {
                id: radioButtonDelegate

                required property MenuItem item
                required property int index

                property string menuId: Boolean(item) ? item.id : ""
                property string title: Boolean(item) ? item.title : ""
                property bool isMenuOpened: menuLoader.isMenuOpened && menuLoader.parent === this

                buttonType: FlatButton.TextOnly
                isNarrow: true
                margins: 8
                drawFocusBorderInsideRect: true

                transparent: !isMenuOpened
                accentButton: isMenuOpened

                navigation.accessible.ignored: true

                visible: mapToItem(root, 0, 0).x + width < root.availableWidth

                AccessibleItem {
                    id: accessibleInfo
                    accessibleParent: panelAccessibleInfo
                    visualItem: radioButtonDelegate
                    role: MUAccessible.Button
                    name: radioButtonDelegate.title
                }

                contentItem: StyledTextLabel {
                    id: textLabel
                    text: radioButtonDelegate.title
                    textFormat: Text.RichText
                    font: ui.theme.bodyFont
                }

                backgroundItem: AppButtonBackground {
                    mouseArea: radioButtonDelegate.mouseArea
                    color: radioButtonDelegate.normalColor
                }

                mouseArea.onHoveredChanged: {
                    if (!mouseArea.containsMouse) {
                        return
                    }
                    if (menuLoader.isMenuOpened && menuLoader.parent !== this) {
                        appMenuModel.openMenu(radioButtonDelegate.menuId, true)
                    }
                }

                onClicked: {
                    appMenuModel.openMenu(radioButtonDelegate.menuId, false)
                }
            }
        }
    }

    StyledMenuLoader {
        id: menuLoader

        property string menuId: ""
        property bool hasSiblingMenus: true

        onHandleMenuItem: function(itemId) {
            Qt.callLater(appMenuModel.handleMenuItem, itemId)
        }

        onOpened: {
            appMenuModel.openedMenuId = menuLoader.menuId
        }

        onClosed: {
            appMenuModel.openedMenuId = ""
        }
    }

    QtObject {
        id: prv

        function openMenu(menuId, byHover) {
            var children = contentRow.children
            for (var i = 0; i < children.length; ++i) {
                var item = children[i]
                if (Boolean(item) && item.menuId === menuId) {
                    if (!byHover) {
                        if (menuLoader.isMenuOpened && menuLoader.parent === item) {
                            menuLoader.close()
                            return
                        }
                    }
                    menuLoader.menuId = menuId
                    menuLoader.parent = item
                    menuLoader.accessibleName = item.title
                    Qt.callLater(menuLoader.open, item.item.subitems)
                    return
                }
            }
        }

        function openedArea(menuLoader) {
            if (menuLoader.isMenuOpened) {
                if (menuLoader.menu.subMenuLoader && menuLoader.menu.subMenuLoader.isMenuOpened)
                    return openedArea(menuLoader.menu.subMenuLoader)
                return Qt.rect(menuLoader.menu.x, menuLoader.menu.y, menuLoader.menu.width, menuLoader.menu.height)
            }
            return Qt.rect(0, 0, 0, 0)
        }
    }
}
