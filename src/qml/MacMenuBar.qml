/*
 * This file is part of Orchestrion.
 *
 * Adapted from MuseScore's PlatformMenuBar.qml,
 * Copyright (C) 2021 MuseScore Limited
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
import Qt.labs.platform 1.1 as PLATFORM

import MuseScore.AppShell
import Orchestrion.MuseScoreShell 1.0

// The native macOS menu bar. A local adaptation of MuseScore's
// PlatformMenuBar.qml: upstream's itemsChanged handler only pours the new
// items' subitems into the existing native menus index by index, so a menu
// appearing or disappearing mid-session (e.g. "Grading" when grading gets
// exposed) shifted every following menu's contents under the old titles.
// Here the menu bar is rebuilt from scratch instead.
Item {
    id: root

    readonly property bool available: menuModel.isGlobalMenuAvailable()

    PLATFORM.MenuBar {
        id: menuBar
    }

    OrchestrionMenuModel {
        id: menuModel
    }

    function load() {
        menuModel.load()
        rebuild()
        menuModel.itemsChanged.connect(rebuild)
    }

    function rebuild() {
        var items = menuModel.items

        // Update in place, reusing the existing native menus: menus newly
        // added to a live bar only attach natively on the next app
        // activation (QCocoaMenuBar::needsImmediateUpdate), so wholesale
        // recreation would leave the bar in limbo until then.
        while (menuBar.menus.length > items.length) {
            var surplus = menuBar.menus[menuBar.menus.length - 1]
            menuBar.removeMenu(surplus)
            surplus.destroy()
        }

        for (var i = 0; i < items.length; ++i) {
            var item = items[i]
            var menu
            if (i < menuBar.menus.length) {
                menu = menuBar.menus[i]
            } else {
                menu = menuComponent.createObject(menuBar)
                menuBar.addMenu(menu)
            }
            setUpMenu(menu, item)

            menu.load()

            item.subitemsChanged.connect(function(subitems, menuId) {
                for (var l in menuBar.menus) {
                    var menu = menuBar.menus[l]
                    if (menu.id === menuId) {
                        menu.subitems = subitems
                        menu.load()
                    }
                }
            })
        }
    }

    function makeMenu(menuInfo) {
        var menu = menuComponent.createObject(menuBar)

        setUpMenu(menu, menuInfo)

        return menu
    }

    function setUpMenu(menu, menuInfo) {
        menu.id = menuInfo.id
        menu.title = menuInfo.title
        menu.enabled = menuInfo.enabled
        menu.subitems = menuInfo.subitems
    }

    function makeMenuItem(parentMenu, itemInfo) {
        var menuItem = menuItemComponent.createObject(parentMenu)

        setUpMenuItem(menuItem, itemInfo)

        return menuItem
    }

    function setUpMenuItem(menuItem, itemInfo) {
        menuItem.id = itemInfo.id
        menuItem.text = itemInfo.title + "\t" + itemInfo.portableShortcuts
        menuItem.enabled = itemInfo.enabled
        menuItem.checked = itemInfo.checked
        menuItem.checkable = itemInfo.checkable
        menuItem.separator = !Boolean(itemInfo.title)
        menuItem.role = itemInfo.role
    }

    Component {
        id: menuComponent

        PLATFORM.Menu {
            property string id: ""
            property var subitems: []

            function load() {
                clear()

                for (var i in subitems) {
                    var item = subitems[i]
                    if (!Boolean(item)) {
                        continue
                    }

                    var isMenu = Boolean(item.subitems) && item.subitems.length > 0

                    if (isMenu) {
                        let menu = root.makeMenu(item)
                        addMenu(menu)
                        menu.load()
                    } else {
                        let menuItem = root.makeMenuItem(this, item)
                        addItem(menuItem)
                    }
                }
            }

            function update() {
                for (var i in subitems) {
                    let item = subitems[i]
                    let isMenu = Boolean(item.subitems) && item.subitems.length > 0

                    if (isMenu) {
                        root.setUpMenu(items[i].subMenu, item)
                    } else {
                        root.setUpMenuItem(items[i], item)
                    }
                }
            }

            onAboutToShow: {
                update()
            }
        }
    }

    Component {
        id: menuItemComponent

        PLATFORM.MenuItem {
            property string id: ""

            onTriggered: {
                Qt.callLater(menuModel.handleMenuItem, id)
            }
        }
    }
}
