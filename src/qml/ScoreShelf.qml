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

import Orchestrion 1.0
import Orchestrion.OrchestrionShell 1.0

// The score shelf: every score one can open at a glance, as cards showing the
// beginning of their music — the recently opened ones, those in the folders
// the user added, then the bundled examples — with a fuzzy search over them
// and the ways to find more. Covers the score view; opened by clicking the
// score's title, Ctrl+O or File ▸ Browse scores…, closed by picking a score,
// Escape or the cross. Folders (or a score) can be dropped on it.
//
// The heading, search box and links stay put; the cards scroll beneath them.
//
// Instantiate with `anchors.fill: parent`; call open() / close().
Item {
    id: root

    property ScoreShelfModel model

    property bool shown: false

    // Read by the computer-keyboard gesture controller: keys pressed in the
    // shelf choose a score, they don't play the one underneath.
    readonly property bool suppressesGestureKeys: shown

    function open() { root.shown = true }
    function close() {
        // Nowhere to go back to without a score.
        if (root.model.hasScore)
            root.shown = false
    }

    visible: opacity > 0
    opacity: shown ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 200 } }

    // Rendering missing cards blocks the GUI thread: only while shown.
    Binding {
        target: root.model
        property: "active"
        value: root.shown
    }

    // Keyboard navigation of the cards: the arrows move through each
    // section's grid, and on from one section to the next; up from the top
    // row goes back to the search box.
    function sectionAt(section) { return sectionsRepeater.itemAt(section) }
    function countOf(section) {
        const shown = sectionAt(section)
        return shown && shown.visible ? shown.cards.count : 0
    }
    // The next section with cards in that direction, or -1.
    function nextSection(section, step) {
        for (let s = section + step; s >= 0 && s < sectionsRepeater.count; s += step) {
            if (countOf(s) > 0)
                return s
        }
        return -1
    }
    function focusCard(section, index) {
        const shown = sectionAt(section)
        const card = shown ? shown.cards.itemAt(index) : null
        if (card)
            card.forceActiveFocus(Qt.TabFocusReason)
    }
    function focusFirstCard() {
        const first = nextSection(-1, 1)
        if (first >= 0)
            focusCard(first, 0)
    }
    function moveFocus(section, index, dx, dy) {
        const columns = flick.columns
        const count = countOf(section)
        if (dx !== 0) {
            const next = index + dx
            if (next < 0) {
                const before = nextSection(section, -1)
                if (before >= 0)
                    focusCard(before, countOf(before) - 1)
            } else if (next >= count) {
                const after = nextSection(section, 1)
                if (after >= 0)
                    focusCard(after, 0)
            } else {
                focusCard(section, next)
            }
            return
        }
        const column = index % columns
        const row = Math.floor(index / columns)
        const lastRow = Math.floor((count - 1) / columns)
        if (dy > 0) {
            if (index + columns < count) {
                focusCard(section, index + columns)
            } else if (row < lastRow) { // a shorter last row: its last card
                focusCard(section, count - 1)
            } else {
                const after = nextSection(section, 1)
                if (after >= 0)
                    focusCard(after, Math.min(column, countOf(after) - 1))
            }
        } else {
            if (index >= columns) {
                focusCard(section, index - columns)
            } else {
                const before = nextSection(section, -1)
                if (before >= 0) {
                    const above = countOf(before)
                    const lastRowStart = Math.floor((above - 1) / columns) * columns
                    focusCard(before, Math.min(lastRowStart + column, above - 1))
                } else {
                    searchField.forceActiveFocus(Qt.TabFocusReason)
                }
            }
        }
    }
    // Scrolls the focused card into view, clear of the fades; with its
    // section's label when it is in the first row.
    function ensureVisible(card) {
        const shown = sectionAt(card.section)
        const top = card.index < flick.columns && shown
            ? shown.y + shown.label.y
            : card.mapToItem(content, 0, 0).y
        const bottom = card.mapToItem(content, 0, card.height).y
        const fade = cardsArea.fadeHeight
        let target = flick.contentY
        if (content.y + top - fade < target)
            target = content.y + top - fade
        else if (content.y + bottom + fade > target + flick.height)
            target = content.y + bottom + fade - flick.height
        target = Math.max(0, Math.min(target, flick.contentHeight - flick.height))
        if (target !== flick.contentY) {
            scrollAnimation.to = target
            scrollAnimation.restart()
        }
    }

    // The cards in view whose music isn't drawn yet go first.
    function prioritizeVisible() {
        const paths = []
        for (let s = 0; s < sectionsRepeater.count; ++s) {
            const shown = sectionAt(s)
            if (!shown || !shown.visible)
                continue
            for (let i = 0; i < shown.cards.count; ++i) {
                const card = shown.cards.itemAt(i)
                if (!card || card.ready)
                    continue
                const y = card.mapToItem(flick, 0, 0).y
                if (y + card.height > 0 && y < flick.height)
                    paths.push(card.path)
            }
        }
        root.model.prioritize(paths)
    }
    Timer {
        id: priorityTimer
        interval: 150
        onTriggered: root.prioritizeVisible()
    }
    Connections {
        target: root.model
        function onQueryChanged() { priorityTimer.restart() }
        function onShownCountChanged() { priorityTimer.restart() }
        function onCloseRequested() { root.shown = false }
    }

    // A fresh search each time, typed straight away.
    onShownChanged: {
        if (shown) {
            searchField.text = ""
            flick.contentY = 0
            priorityTimer.restart()
            searchField.forceActiveFocus()
        } else {
            // Else the keys would keep going to an invisible text field
            // instead of playing.
            searchField.focus = false
        }
    }

    // A page is what shows between the fades, so that the cards half hidden
    // under one come out whole.
    function scrollPage(direction) {
        const page = flick.height - 2 * cardsArea.fadeHeight
        const from = scrollAnimation.running ? scrollAnimation.to : flick.contentY
        const target = Math.max(0, Math.min(from + direction * page,
                                             flick.contentHeight - flick.height))
        if (target !== flick.contentY) {
            scrollAnimation.to = target
            scrollAnimation.restart()
        }
    }
    Shortcut {
        sequences: [StandardKey.MoveToPreviousPage]
        enabled: root.shown
        onActivated: root.scrollPage(-1)
    }
    Shortcut {
        sequences: [StandardKey.MoveToNextPage]
        enabled: root.shown
        onActivated: root.scrollPage(1)
    }

    Shortcut {
        sequences: [StandardKey.Find]
        enabled: root.shown
        onActivated: {
            searchField.forceActiveFocus(Qt.ShortcutFocusReason)
            searchField.selectAll()
        }
    }

    // Clears the search first, if there is one.
    Shortcut {
        sequence: "Escape"
        enabled: root.shown
        onActivated: {
            if (searchField.text.length > 0)
                searchField.text = ""
            else
                root.close()
        }
    }

    // The card's strip, as rendered by ScoreIncipitCache.
    readonly property int stripWidth: 320
    readonly property int stripHeight: 112
    readonly property int cardPadding: 8
    readonly property int cardWidth: stripWidth + 2 * cardPadding
    readonly property int cardSpacing: 20
    readonly property int cardHeight: cardPadding + stripHeight + 10
        + titleMetrics.height + 3 + composerMetrics.height + cardPadding + 4

    FontMetrics {
        id: titleMetrics
        font.pixelSize: 15
        font.weight: Font.DemiBold
    }
    FontMetrics {
        id: composerMetrics
        font.pixelSize: 12
    }

    FontLoader {
        id: titleFont
        source: "qrc:/fonts/CinzelDecorative/CinzelDecorative-Regular.ttf"
    }

    // Opaque: the score underneath would show through the cards' gaps.
    Rectangle {
        anchors.fill: parent
        color: Theme.backdrop
    }

    // Nothing underneath gets the mouse while the shelf is up.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        hoverEnabled: true
        onWheel: function(wheel) { wheel.accepted = true }
    }

    // Keys the shelf's controls handle, which would otherwise trigger the
    // application's shortcuts (Space plays, the arrows step through the
    // score).
    function isNavigationKey(key) {
        return key === Qt.Key_Space || key === Qt.Key_Return
            || key === Qt.Key_Enter || key === Qt.Key_Left
            || key === Qt.Key_Right || key === Qt.Key_Up
            || key === Qt.Key_Down
    }

    component SectionLabel: Text {
        color: Theme.inkMuted
        font.family: titleFont.name
        font.pixelSize: 15
        font.letterSpacing: 2
    }

    // A link to elsewhere: the file dialog, musescore.com, the wiki.
    component ShelfLink: Rectangle {
        id: link

        property alias text: label.text
        signal clicked()

        readonly property bool lit: linkMouse.containsMouse || activeFocus

        width: label.implicitWidth + 32
        height: label.implicitHeight + 14
        radius: height / 2
        color: "transparent"
        border.color: link.lit ? Theme.metalBright : Theme.metal
        border.width: activeFocus ? 2 : 1

        activeFocusOnTab: true
        Keys.onShortcutOverride: function(event) {
            event.accepted = event.key === Qt.Key_Space
                || event.key === Qt.Key_Return || event.key === Qt.Key_Enter
        }
        Keys.onReturnPressed: link.clicked()
        Keys.onEnterPressed: link.clicked()
        Keys.onSpacePressed: link.clicked()
        Keys.onDownPressed: root.focusFirstCard()

        Text {
            id: label
            anchors.centerIn: parent
            color: link.lit ? Theme.metalBright : Theme.metal
            font.pixelSize: 14
        }

        MouseArea {
            id: linkMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: link.clicked()
        }
    }

    component ScoreCard: Rectangle {
        id: card

        // The model's roles.
        required property string title
        required property string composer
        required property string incipit
        required property bool ready
        required property string path
        required property string nativePath
        required property string displayNameOverride
        required property bool current
        required property int index
        // 0: Play again, 1: Examples.
        property int section: 0

        readonly property bool lit: cardMouse.containsMouse || activeFocus

        function choose() {
            // The score that is open: just back to it.
            if (!card.current)
                root.model.open(card.path, card.displayNameOverride)
            root.shown = false
        }

        width: root.cardWidth
        height: root.cardHeight
        radius: 8
        color: Theme.popup
        border.color: card.current || card.lit ? Theme.metalBright : Theme.metal
        border.width: card.current || card.activeFocus ? 2 : 1
        scale: card.lit ? 1.03 : 1
        Behavior on scale { NumberAnimation { duration: 120 } }

        activeFocusOnTab: true
        onActiveFocusChanged: {
            if (activeFocus)
                root.ensureVisible(card)
        }
        Keys.onShortcutOverride: function(event) {
            event.accepted = root.isNavigationKey(event.key)
        }
        Keys.onReturnPressed: card.choose()
        Keys.onEnterPressed: card.choose()
        Keys.onSpacePressed: card.choose()
        Keys.onLeftPressed: root.moveFocus(card.section, card.index, -1, 0)
        Keys.onRightPressed: root.moveFocus(card.section, card.index, 1, 0)
        Keys.onUpPressed: root.moveFocus(card.section, card.index, 0, -1)
        Keys.onDownPressed: root.moveFocus(card.section, card.index, 0, 1)
        // Typing searches.
        Keys.onPressed: function(event) {
            if (event.text.length > 0 && event.text.trim().length > 0
                && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier
                                        | Qt.MetaModifier))) {
                searchField.forceActiveFocus()
                searchField.insert(searchField.length, event.text)
                event.accepted = true
            }
        }

        // The parchment slip with the beginning of the music, fading out to
        // the right as if it went on.
        Rectangle {
            id: slip
            x: root.cardPadding
            y: root.cardPadding
            width: root.stripWidth
            height: root.stripHeight
            radius: 4
            color: Theme.accent
            clip: true

            // While the music is being drawn: a bare staff, breathing.
            Column {
                visible: !card.ready
                anchors.centerIn: parent
                spacing: 6
                SequentialAnimation on opacity {
                    running: !card.ready && root.shown
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.12; to: 0.3; duration: 700 }
                    NumberAnimation { from: 0.3; to: 0.12; duration: 700 }
                }
                Repeater {
                    model: 5
                    Rectangle {
                        width: slip.width - 32
                        height: 1
                        color: Theme.accentInk
                    }
                }
            }

            Image {
                anchors.fill: parent
                source: card.incipit
                sourceSize.width: root.stripWidth * 2
                sourceSize.height: root.stripHeight * 2
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
                asynchronous: true
                // Fades in when rendered.
                opacity: status === Image.Ready ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 250 } }
            }

            Rectangle {
                anchors.right: parent.right
                width: parent.width * 0.3
                height: parent.height
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0; color: "transparent" }
                    GradientStop { position: 1; color: Theme.accent }
                }
            }
        }

        Column {
            id: textColumn
            x: root.cardPadding + 2
            anchors.top: slip.bottom
            anchors.topMargin: 10
            width: root.stripWidth - 4
            spacing: 3

            Text {
                width: parent.width
                text: (card.current ? "◆  " : "") + card.title
                color: Theme.metalBright
                font.pixelSize: 15
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                // Kept when empty, so that all cards are the same height.
                text: card.current
                      ? (card.composer.length > 0
                         ? qsTr("Now playing · %1").arg(card.composer)
                         : qsTr("Now playing"))
                      : card.composer.length > 0 ? card.composer : " "
                color: Theme.inkMuted
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }

        MouseArea {
            id: cardMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.choose()
        }
        OrchestrionToolTip {
            visible: cardMouse.containsMouse
            text: card.nativePath
        }
    }

    // The way to the user's own scores, at the end of the examples until a
    // folder has been added.
    component AddFolderCard: Item {
        id: addCard

        readonly property bool lit: addMouse.containsMouse || activeFocus

        width: root.cardWidth
        height: root.cardHeight
        scale: addCard.lit ? 1.03 : 1
        Behavior on scale { NumberAnimation { duration: 120 } }

        activeFocusOnTab: true
        Keys.onShortcutOverride: function(event) {
            event.accepted = root.isNavigationKey(event.key)
        }
        Keys.onReturnPressed: root.model.addFolder()
        Keys.onEnterPressed: root.model.addFolder()
        Keys.onSpacePressed: root.model.addFolder()

        // A dashed outline: a place for cards still to come.
        Canvas {
            id: outline
            anchors.fill: parent
            readonly property color ink: addCard.lit ? Theme.metalBright : Theme.metal
            readonly property real lineWidth: addCard.activeFocus ? 2 : 1
            onInkChanged: requestPaint()
            onLineWidthChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = ink
                ctx.lineWidth = lineWidth
                ctx.setLineDash([6, 4])
                const inset = lineWidth / 2
                ctx.beginPath()
                ctx.roundedRect(inset, inset, width - lineWidth,
                                height - lineWidth, 8, 8)
                ctx.stroke()
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 6

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "+"
                color: outline.ink
                font.pixelSize: 32
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Add a folder of your own scores")
                color: Theme.metalBright
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Its scores will be listed here, and searched")
                color: Theme.inkMuted
                font.pixelSize: 12
            }
        }

        MouseArea {
            id: addMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.model.addFolder()
        }
    }

    Column {
        id: header
        anchors.top: parent.top
        anchors.topMargin: 24
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 18

        BackdropOrnament {
            anchors.horizontalCenter: parent.horizontalCenter
            viewWidth: root.width
            viewHeight: root.height
            title: qsTr("Choose a score")
        }

        // One row when the window is wide enough; wraps when not.
        Flow {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(root.width - 48,
                            searchField.width + fileLink.width + folderLink.width
                            + onlineLink.width + helpLink.width + 4 * spacing)
            spacing: 12
            topPadding: 4

            // Fuzzy: the letters in order, not necessarily together ("mzrt"
            // finds Mozart); Return opens the best match.
            TextField {
                id: searchField
                width: 280
                height: 36
                leftPadding: 38
                rightPadding: 32
                placeholderText: qsTr("Search scores")
                placeholderTextColor: Theme.inkFaint
                color: Theme.metalBright
                selectionColor: Theme.metal
                selectedTextColor: Theme.backdrop
                font.pixelSize: 14
                verticalAlignment: TextInput.AlignVCenter
                onTextChanged: root.model.query = text
                Keys.onDownPressed: root.focusFirstCard()
                onAccepted: {
                    if (root.model.openBestMatch())
                        root.shown = false
                }

                background: Rectangle {
                    radius: height / 2
                    color: "transparent"
                    border.color: searchField.activeFocus ? Theme.metalBright
                                                          : Theme.metal
                    border.width: 1
                }

                // A magnifying glass: a ring and its handle, the handle's
                // axis through the ring's centre at 45°.
                Item {
                    id: magnifier
                    x: 14
                    anchors.verticalCenter: parent.verticalCenter
                    width: 14
                    height: 14

                    readonly property real ringDiameter: 10
                    readonly property real stroke: 1.5
                    readonly property real handleLength: 6
                    readonly property real centre: ringDiameter / 2
                    // The handle starts on the middle of the ring's stroke,
                    // so that the two join without a seam.
                    readonly property real handleStart:
                        centre + (centre - stroke / 2) * Math.SQRT1_2

                    Rectangle {
                        width: magnifier.ringDiameter
                        height: magnifier.ringDiameter
                        radius: magnifier.centre
                        color: "transparent"
                        border.color: Theme.metal
                        border.width: magnifier.stroke
                        antialiasing: true
                    }
                    // Rotated about the middle of its starting end, which is
                    // where it is placed.
                    Rectangle {
                        x: magnifier.handleStart
                        y: magnifier.handleStart - height / 2
                        width: magnifier.handleLength
                        height: magnifier.stroke
                        rotation: 45
                        transformOrigin: Item.Left
                        color: Theme.metal
                        antialiasing: true
                    }
                }

                Text {
                    visible: searchField.text.length > 0
                    anchors.right: parent.right
                    anchors.rightMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    text: "✕"
                    color: clearMouse.containsMouse ? Theme.metalBright
                                                    : Theme.inkFaint
                    font.pixelSize: 12

                    MouseArea {
                        id: clearMouse
                        anchors.fill: parent
                        anchors.margins: -6
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: searchField.text = ""
                    }
                }
            }

            ShelfLink {
                id: fileLink
                height: searchField.height
                text: qsTr("Open from computer…")
                onClicked: {
                    root.shown = false
                    root.model.openFromComputer()
                }
            }
            ShelfLink {
                id: folderLink
                height: searchField.height
                text: qsTr("Add a folder…")
                onClicked: root.model.addFolder()
            }
            ShelfLink {
                id: onlineLink
                height: searchField.height
                text: qsTr("Search on musescore.com")
                onClicked: root.model.searchOnline()
            }
            ShelfLink {
                id: helpLink
                height: searchField.height
                text: qsTr("Help me find scores…")
                onClicked: root.model.helpFindScores()
            }
        }
    }

    // The cards, scrolling on their own under the header.
    Item {
        id: cardsArea
        anchors.top: header.bottom
        anchors.topMargin: 12
        anchors.bottom: parent.bottom
        // The same air below as the header has above.
        anchors.bottomMargin: header.anchors.topMargin
        anchors.left: parent.left
        anchors.right: parent.right

        readonly property bool empty: root.model.shownCount === 0
                                      && root.model.query.length > 0

        Flickable {
            id: flick
            anchors.fill: parent
            contentWidth: width
            contentHeight: content.height + 2 * cardsArea.fadeHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            onContentYChanged: priorityTimer.restart()

            NumberAnimation on contentY {
                id: scrollAnimation
                running: false
                duration: 150
                easing.type: Easing.OutQuad
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
                width: 10
                rightPadding: 4
                contentItem: Rectangle {
                    implicitWidth: 6
                    radius: 3
                    color: Theme.metal
                    opacity: parent.pressed ? 1 : parent.hovered ? 0.85 : 0.6
                }
                background: Rectangle {
                    x: parent.width - 4 - width
                    implicitWidth: 6
                    width: 6
                    radius: 3
                    color: Theme.inkFaint
                    opacity: 0.15
                }
            }

            // As many cards a row as fit, the rows centred.
            readonly property int columns: Math.max(1, Math.floor(
                (width - 48 + root.cardSpacing) / (root.cardWidth + root.cardSpacing)))
            readonly property int rowWidth:
                columns * root.cardWidth + (columns - 1) * root.cardSpacing

            Column {
                id: content
                x: (flick.width - flick.rowWidth) / 2
                // Clear of the fade when scrolled to either end.
                y: cardsArea.fadeHeight
                width: flick.rowWidth
                spacing: 30

                Repeater {
                    id: sectionsRepeater
                    model: root.model.sections

                    delegate: Column {
                        id: shelfSection

                        required property int index
                        required property string kind
                        required property string title
                        required property string subtitle
                        required property string path
                        required property QtObject scores

                        readonly property alias cards: cardRepeater
                        readonly property alias label: labelRow
                        readonly property bool searching: root.model.query.length > 0
                        readonly property bool isFolder: kind === "folder"
                        readonly property bool showsAddCard: kind === "examples"
                            && !root.model.hasFolders && !searching

                        // A folder keeps its section while not searching,
                        // even empty: that's where its scores will come.
                        visible: scores.count > 0 || (isFolder && !searching)
                                 || showsAddCard
                        width: parent.width
                        spacing: 18

                        // The section's label; a folder's can be
                        // right-clicked for what can be done with it.
                        Item {
                            id: labelRow
                            width: labelContent.implicitWidth
                            height: labelContent.implicitHeight

                            MouseArea {
                                anchors.fill: parent
                                enabled: shelfSection.isFolder
                                acceptedButtons: Qt.RightButton
                                onClicked: folderMenu.popup()
                            }

                            Row {
                                id: labelContent
                                spacing: 12

                                SectionLabel {
                                    id: sectionLabel
                                    text: shelfSection.title
                                }
                                Text {
                                    visible: shelfSection.isFolder
                                    anchors.baseline: sectionLabel.baseline
                                    text: shelfSection.subtitle
                                    color: Theme.inkFaint
                                    font.pixelSize: 12
                                }
                                Text {
                                    visible: shelfSection.isFolder
                                    anchors.verticalCenter: sectionLabel.verticalCenter
                                    text: "✕"
                                    color: removeMouse.containsMouse ? Theme.metalBright
                                                                     : Theme.inkFaint
                                    font.pixelSize: 12

                                    MouseArea {
                                        id: removeMouse
                                        anchors.fill: parent
                                        anchors.margins: -6
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.model.removeFolder(shelfSection.path)
                                    }
                                    OrchestrionToolTip {
                                        visible: removeMouse.containsMouse
                                        text: qsTr("Remove from the shelf (the folder stays on disk)")
                                    }
                                }
                            }
                        }

                        Menu {
                            id: folderMenu
                            MenuItem {
                                text: qsTr("Show in file manager")
                                onTriggered: root.model.revealFolder(shelfSection.path)
                            }
                            MenuItem {
                                text: qsTr("Remove from the shelf")
                                onTriggered: root.model.removeFolder(shelfSection.path)
                            }
                        }

                        Text {
                            visible: shelfSection.isFolder && shelfSection.scores.count === 0
                                     && !shelfSection.searching
                            text: qsTr("No scores in this folder yet")
                            color: Theme.inkMuted
                            font.pixelSize: 14
                        }

                        Flow {
                            width: parent.width
                            spacing: root.cardSpacing
                            visible: shelfSection.scores.count > 0 || shelfSection.showsAddCard

                            Repeater {
                                id: cardRepeater
                                model: shelfSection.scores
                                delegate: ScoreCard { section: shelfSection.index }
                            }

                            AddFolderCard {
                                visible: shelfSection.showsAddCard
                            }
                        }
                    }
                }
            }
        }

        Text {
            visible: cardsArea.empty
            anchors.horizontalCenter: parent.horizontalCenter
            y: cardsArea.fadeHeight + 24
            text: qsTr("No score matches \u201C%1\u201D").arg(searchField.text)
            color: Theme.inkMuted
            font.pixelSize: 15
        }

        // The cards fade in and out at the edges, into the backdrop (opaque,
        // so drawing its colour over them is a fade). The scroll bar's strip
        // is left out, so that it stays crisp.
        readonly property int fadeHeight: 32

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 14
            height: cardsArea.fadeHeight
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.backdrop }
                GradientStop {
                    position: 1
                    color: Qt.rgba(Theme.backdrop.r, Theme.backdrop.g,
                                   Theme.backdrop.b, 0)
                }
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 14
            height: cardsArea.fadeHeight
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: Qt.rgba(Theme.backdrop.r, Theme.backdrop.g,
                                   Theme.backdrop.b, 0)
                }
                GradientStop { position: 1; color: Theme.backdrop }
            }
        }
    }

    // Folders dropped on the shelf are added to it; a score is opened.
    DropArea {
        id: dropArea
        anchors.fill: parent
        enabled: root.shown
        onEntered: function(drag) { drag.accepted = drag.hasUrls }
        onDropped: function(drop) {
            if (root.model.drop(drop.urls))
                drop.acceptProposedAction()
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 12
        visible: dropArea.containsDrag
        radius: 12
        color: Theme.overlay
        border.color: Theme.metalBright
        border.width: 2

        Text {
            anchors.centerIn: parent
            text: qsTr("Drop a folder to add it to the shelf, or a score to open it")
            color: Theme.metalBright
            font.pixelSize: 18
        }
    }

    Text {
        visible: root.model.hasScore
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 14
        anchors.rightMargin: 20
        text: "✕"
        color: closeMouse.containsMouse ? Theme.metalBright : Theme.inkFaint
        font.pixelSize: 20

        MouseArea {
            id: closeMouse
            anchors.fill: parent
            anchors.margins: -8
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.close()
        }
    }
}
