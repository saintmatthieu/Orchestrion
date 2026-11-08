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

// The score shelf: every score one can open at a glance, as a table whose
// rows are cards showing the beginning of their music beside the score's
// title, composer, opus, difficulty and length — the recently opened ones,
// those in the folders the user added, then the library (the bundled scores
// and those of Rolls) — with a fuzzy search over them and the ways to find
// more. Clicking a column's heading sorts the rows by it, the sections merged
// into one; again, the other way round; a third time, back to the sections,
// each in its own order. A
// score of Rolls not yet on this computer has no music on its card, only a
// note saying it is downloaded when opened. Covers the score view; opened by
// clicking the score's title, Ctrl+O or File ▸ Browse scores…, closed by
// picking a score, Escape or the ringed cross. Folders (or a score) can be
// dropped on it.
//
// The heading, search box, links and column headings stay put; the cards
// scroll beneath them.
//
// With `tutorials`, it shows the tutorials only (Help ▸ Tutorials…); otherwise
// the scores only.
//
// Instantiate with `anchors.fill: parent`; call open() / close().
Item {
    id: root

    property ScoreShelfModel model

    property bool shown: false
    // Showing the tutorials only
    property bool tutorialsOnly: false

    // Read by the computer-keyboard gesture controller: keys pressed in the
    // shelf choose a score, they don't play the one underneath.
    readonly property bool suppressesGestureKeys: shown

    function open(tutorials) {
        root.tutorialsOnly = tutorials === true
        root.shown = true
    }
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
        const topFade = cardsArea.topFadeHeight
        const fade = cardsArea.fadeHeight
        let target = flick.contentY
        if (content.y + top - topFade < target)
            target = content.y + top - topFade
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
                    paths.push(card.picturePath)
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

    // The search is kept from one opening to the next, each view's its own
    // (the scores', the tutorials'), but not one a tutorial typed: the user
    // finds theirs. It is selected when the shelf opens, so that typing
    // starts another while the arrows keep it.
    property string scoresQuery: ""
    property string tutorialsQuery: ""
    property string queryBeforeTutorial: ""
    function showQuery() {
        searchField.text = root.tutorialsOnly ? root.tutorialsQuery : root.scoresQuery
        searchField.selectAll()
    }
    onTutorialsOnlyChanged: {
        if (shown)
            showQuery()
    }
    onShownChanged: {
        if (shown) {
            openedByTutorial = Tutorials.running
            if (openedByTutorial)
                queryBeforeTutorial = scoresQuery
            showQuery()
            flick.contentY = 0
            priorityTimer.restart()
            searchField.forceActiveFocus()
        } else {
            if (openedByTutorial) {
                scoresQuery = queryBeforeTutorial
                openedByTutorial = false
            }
            // Else the keys would keep going to an invisible text field
            // instead of playing.
            searchField.focus = false
        }
    }

    // A page is what shows between the fades, so that the cards half hidden
    // under one come out whole.
    function scrollPage(direction) {
        const page = flick.height - cardsArea.topFadeHeight - cardsArea.fadeHeight
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
    // Not during a tutorial, which Esc stops.
    Shortcut {
        sequence: "Escape"
        enabled: root.shown && !Tutorials.running
        onActivated: {
            if (searchField.text.length > 0)
                searchField.text = ""
            else
                root.close()
        }
    }

    // A demo's caption has a row of its own under the cards: what is drawn
    // under the shelf is hidden by it.
    property real captionOpen: DemoKeys.caption.length > 0 ? 1 : 0
    Behavior on captionOpen {
        NumberAnimation { duration: 350; easing.type: Easing.InOutCubic }
    }
    DemoCaption {
        id: demoCaption
        z: 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: header.anchors.topMargin
        maximumWidth: parent.width - 48
        opacity: root.captionOpen
        visible: opacity > 0
    }

    // Opened by a tutorial: put away with it, however it ends
    property bool openedByTutorial: false
    Connections {
        target: Tutorials
        function onRunningStateChanged() {
            if (!Tutorials.running && root.openedByTutorial)
                root.close()
        }
    }

    // The card's strip, as rendered by ScoreIncipitCache, on the row's left.
    readonly property int stripWidth: 320
    readonly property int stripHeight: 112
    readonly property int cardPadding: 8
    readonly property int cardHeight: stripHeight + 2 * cardPadding
    readonly property int cardSpacing: 10
    readonly property int maxRowWidth: 1100

    // The table's columns, from the left of a row: the slip, the title over
    // the composer (taking what is left), then the opus and the difficulty's
    // two levels, each wide enough for its heading and the sort's arrow; the
    // levels end at the row's right edge. Dents in the card's edges mark
    // where a column starts (columnStarts()).
    readonly property int slipColumnWidth: stripWidth + 2 * cardPadding
    readonly property int cellPadding: 16
    readonly property int opusWidth: 140
    readonly property int levelWidth: 72
    readonly property int minTitleWidth: 220
    readonly property int titleX: slipColumnWidth
    // Too narrow a window: the title and composer only, unsorted.
    readonly property bool showsColumns: !tutorialsOnly
        && flick.rowWidth >= titleX + minTitleWidth + opusWidth + 2 * levelWidth
    readonly property int notesX: flick.rowWidth - levelWidth
    readonly property int rhythmX: notesX - levelWidth
    readonly property int opusX: rhythmX - opusWidth
    readonly property int titleWidth:
        (showsColumns ? opusX : flick.rowWidth) - titleX

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
    // Keys the shelf's items take before the application's shortcuts do:
    // Tab, for one, is keyboard navigation's (see shortcuts.json).
    function isNavigationKey(key) {
        return key === Qt.Key_Space || key === Qt.Key_Return
            || key === Qt.Key_Enter || key === Qt.Key_Left
            || key === Qt.Key_Right || key === Qt.Key_Up
            || key === Qt.Key_Down || key === Qt.Key_Tab
            || key === Qt.Key_Backtab
    }

    component SectionLabel: Text {
        color: Theme.inkMuted
        font.family: titleFont.name
        font.pixelSize: 15
        font.letterSpacing: 2
    }
    readonly property int sectionLabelSpacing: 18
    // Measures a section's label, never shown.
    SectionLabel {
        id: sectionLabelProbe
        visible: false
        text: "X"
    }

    // The air between the shelf's blocks, the same all down: under the
    // title's letters, under the search row, under the headings. It is what
    // the headings need to their first row of cards: the cards' clearance
    // and fade, then the first section's label and its spacing.
    readonly property real blockGap: cardsArea.headingsClearance
        + cardsArea.topFadeHeight + sectionLabelProbe.implicitHeight
        + sectionLabelSpacing

    // A link to elsewhere: the file dialog, musescore.com.
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
                || event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab
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

    // A number in a column of the table, blank when not known (0).
    component NumberCell: Text {
        property int value: 0
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignHCenter
        text: value > 0 ? value : ""
        color: Theme.metalBright
        font.pixelSize: 15
        font.features: ({ "tnum": 1 })
    }

    // A card whose top and bottom edges are dented, a small V pointing in,
    // where each of its columns starts (`dents`, x positions in the card),
    // with a hairline rule between the two dents, short of either.
    component DentedCard: Canvas {
        id: dented

        property var dents: []
        property color ink: Theme.metal
        property real lineWidth: 1
        readonly property real radius: 8
        readonly property real dentWidth: 10
        readonly property real dentDepth: 4
        readonly property real ruleGap: 6
        // The rules' opacity: subtler than the edges.
        readonly property real ruleOpacity: 0.1

        onDentsChanged: requestPaint()
        onInkChanged: requestPaint()
        onLineWidthChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            // The stroke's middle, so that all of it is inside.
            const inset = lineWidth / 2
            const left = inset, top = inset
            const right = width - inset, bottom = height - inset
            const r = radius, w = dentWidth / 2, d = dentDepth
            // On a pixel's middle, so that the 1 px rule is crisp; the dents'
            // tips with it.
            const xs = dents.map(x => Math.round(x) + 0.5).sort((a, b) => a - b)
            ctx.beginPath()
            ctx.moveTo(left + r, top)
            for (const x of xs) {
                ctx.lineTo(x - w, top)
                ctx.lineTo(x, top + d)
                ctx.lineTo(x + w, top)
            }
            ctx.arcTo(right, top, right, top + r, r)
            ctx.arcTo(right, bottom, right - r, bottom, r)
            for (const x of xs.slice().reverse()) {
                ctx.lineTo(x + w, bottom)
                ctx.lineTo(x, bottom - d)
                ctx.lineTo(x - w, bottom)
            }
            ctx.arcTo(left, bottom, left, bottom - r, r)
            ctx.arcTo(left, top, left + r, top, r)
            ctx.closePath()
            ctx.fillStyle = Theme.popup
            ctx.fill()
            ctx.lineWidth = lineWidth
            ctx.lineJoin = "round"
            ctx.strokeStyle = ink
            ctx.stroke()

            ctx.beginPath()
            for (const x of xs) {
                ctx.moveTo(x, top + d + ruleGap)
                ctx.lineTo(x, bottom - d - ruleGap)
            }
            ctx.lineWidth = 1
            ctx.strokeStyle = Theme.metal
            ctx.globalAlpha = ruleOpacity
            ctx.stroke()
            ctx.globalAlpha = 1
        }
    }

    // A column's heading, which sorts by it when clicked; an arrow beside it
    // says it does, and which way. The arrow hangs off the label, so that
    // the label stays aligned with its column.
    component SortHeading: Item {
        id: heading

        property string key
        property alias text: label.text
        property alias tip: headingTip.text
        property bool centred: false
        // Set like the rows' titles (bold, bright gold), for the Title's
        // heading; else small and muted, lit when hovered or sorted.
        property bool asTitle: false

        readonly property bool sorted: root.model.sortKey === key
        readonly property bool lit: headingMouse.containsMouse || activeFocus

        function sort() { root.model.sortBy(heading.key) }

        implicitWidth: label.implicitWidth + arrow.width + 4
        implicitHeight: label.implicitHeight
        // So that headings of different sizes can share a baseline.
        baselineOffset: label.y + label.baselineOffset

        activeFocusOnTab: true
        Accessible.role: Accessible.ColumnHeader
        Accessible.name: label.text
        Accessible.onPressAction: heading.sort()
        Keys.onShortcutOverride: function(event) {
            event.accepted = event.key === Qt.Key_Space
                || event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                || event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab
        }
        Keys.onReturnPressed: heading.sort()
        Keys.onEnterPressed: heading.sort()
        Keys.onSpacePressed: heading.sort()
        Keys.onDownPressed: root.focusFirstCard()

        Text {
            id: label
            x: heading.centred ? (heading.width - width) / 2 : 0
            color: heading.asTitle || heading.lit || heading.sorted ? Theme.metalBright
                                                                    : Theme.inkMuted
            font.pixelSize: heading.asTitle ? 17 : 13
            font.weight: heading.asTitle || heading.sorted ? Font.DemiBold : Font.Normal
            font.underline: heading.activeFocus
        }
        // Drawn rather than a glyph, which a fallback font would place: a
        // triangle in the box of the label's x-height (a little less), on
        // its baseline; odd-wide, so that its point is on a pixel's middle.
        FontMetrics {
            id: labelMetrics
            font: label.font
        }
        Canvas {
            id: arrow
            readonly property bool down: root.model.sortDescending
            readonly property color ink: Theme.metalBright
            x: label.x + label.width + 4
            y: label.y + label.baselineOffset - height
            height: Math.round(labelMetrics.xHeight * 0.85)
            width: height + 1
            visible: heading.sorted
            onDownChanged: requestPaint()
            onInkChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.beginPath()
                if (down) {
                    ctx.moveTo(0, 0)
                    ctx.lineTo(width, 0)
                    ctx.lineTo(width / 2, height)
                } else {
                    ctx.moveTo(0, height)
                    ctx.lineTo(width, height)
                    ctx.lineTo(width / 2, 0)
                }
                ctx.closePath()
                ctx.fillStyle = ink
                ctx.fill()
            }
        }
        MouseArea {
            id: headingMouse
            anchors.fill: label
            anchors.margins: -4
            anchors.rightMargin: -4 - arrow.width
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: heading.sort()
        }
        OrchestrionToolTip {
            id: headingTip
            visible: headingMouse.containsMouse && text.length > 0
        }
    }

    component ScoreCard: Rectangle {
        id: card
        objectName: "demo:Shelf/Card"

        // The model's roles.
        required property string title
        required property string composer
        required property string incipit
        required property bool ready
        required property string path
        required property string nativePath
        required property string displayNameOverride
        required property bool current
        required property bool downloaded
        required property bool downloading
        required property bool downloadFailed
        required property bool updateAvailable
        required property bool tutorial
        required property string picturePath
        required property string opus
        required property int rhythm
        required property int notes
        required property int index
        // The index of its section.
        property int section: 0

        readonly property bool lit: cardMouse.containsMouse || activeFocus

        function choose() {
            if (card.tutorial) {
                root.shown = false
                root.model.runTutorial(card.path)
                return
            }
            // The score that is open: just back to it. One still to be
            // downloaded: the shelf closes once it is, and it opens.
            if (card.current || root.model.open(card.path, card.displayNameOverride))
                root.shown = false
        }

        width: flick.rowWidth
        height: root.cardHeight
        color: "transparent"

        // The title's column starts after the slip's; the opus's and the
        // difficulty's, unless there is nothing in them (a tutorial).
        DentedCard {
            anchors.fill: parent
            dents: root.showsColumns && !card.tutorial
                   ? [root.titleX, root.opusX, root.rhythmX] : [root.titleX]
            // Not lit for being the score that is open (its ◆ and "Now
            // playing" say it): it would look focused next to the row that is.
            ink: card.lit ? Theme.metalBright : Theme.metal
            lineWidth: card.activeFocus ? 2 : 1
        }

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

            // A bare staff, breathing while the music is being drawn.
            Column {
                id: bareStaff
                readonly property bool breathing: !card.ready && root.shown
                property real breath: 0.12
                visible: !card.ready && card.downloaded
                anchors.centerIn: parent
                spacing: 6
                opacity: breathing ? breath : 0.12
                SequentialAnimation on breath {
                    running: bareStaff.breathing
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

            // Not on this computer: no music to show, so say where it is
            // rather than draw empty staves, which look broken. Breathes
            // while it downloads.
            Column {
                id: remoteNote
                property real breath: 0.55
                visible: !card.downloaded
                anchors.centerIn: parent
                spacing: 4
                opacity: card.downloading && root.shown ? breath : 0.55
                SequentialAnimation on breath {
                    running: card.downloading && root.shown
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.55; to: 0.25; duration: 700 }
                    NumberAnimation { from: 0.25; to: 0.55; duration: 700 }
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Rolls"
                    color: Theme.accentInk
                    font.family: titleFont.name
                    font.pixelSize: 20
                    font.letterSpacing: 2
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: card.downloading ? qsTr("Downloading…")
                                           : qsTr("Downloaded when you open it")
                    color: Theme.accentInk
                    font.pixelSize: 12
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

            // A tutorial plays that music: a play badge over it.
            Rectangle {
                visible: card.tutorial
                anchors.centerIn: parent
                width: 52
                height: 52
                radius: width / 2
                color: Theme.backdrop
                opacity: card.lit ? 0.9 : 0.75
                border.color: Theme.metalBright
                border.width: 2

                Canvas {
                    anchors.centerIn: parent
                    // Optically centred: the triangle's weight is left of its box.
                    anchors.horizontalCenterOffset: 2
                    width: 20
                    height: 22
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.reset()
                        ctx.beginPath()
                        ctx.moveTo(0, 0)
                        ctx.lineTo(width, height / 2)
                        ctx.lineTo(0, height)
                        ctx.closePath()
                        ctx.fillStyle = Theme.metalBright
                        ctx.fill()
                    }
                }
            }
        }

        Column {
            id: textColumn
            x: root.titleX + root.cellPadding
            anchors.verticalCenter: parent.verticalCenter
            // A tutorial has nothing in the other columns: its description
            // may run over them.
            width: (card.tutorial ? card.width - root.titleX : root.titleWidth)
                   - 2 * root.cellPadding
            spacing: 4

            Text {
                width: parent.width
                text: (card.current ? "◆  " : "") + card.title
                color: Theme.metalBright
                font.pixelSize: 17
                font.weight: Font.DemiBold
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                visible: text.length > 0
                text: card.downloadFailed
                      ? qsTr("Couldn't download it: are you online?")
                      : card.current
                      ? (card.composer.length > 0
                         ? qsTr("Now playing · %1").arg(card.composer)
                         : qsTr("Now playing"))
                      : card.composer
                color: Theme.inkMuted
                font.pixelSize: 14
                wrapMode: card.tutorial ? Text.Wrap : Text.NoWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        // The columns; blank where not known (the difficulty and opus of a
        // score that doesn't say them).
        Item {
            visible: root.showsColumns && !card.tutorial
            anchors.fill: parent

            Text {
                x: root.opusX + 8
                width: root.opusWidth - 16
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                text: card.opus
                color: Theme.inkMuted
                font.pixelSize: 14
                elide: Text.ElideRight
            }
            NumberCell {
                x: root.rhythmX
                width: root.levelWidth
                value: card.rhythm
            }
            NumberCell {
                x: root.notesX
                width: root.levelWidth
                value: card.notes
            }
        }

        MouseArea {
            id: cardMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function(mouse) {
                if (mouse.button === Qt.RightButton) {
                    // Only a score on this computer can be shown.
                    if (card.downloaded && !card.tutorial)
                        cardMenu.popup()
                } else {
                    card.choose()
                }
            }
        }
        Menu {
            id: cardMenu
            MenuItem {
                text: qsTr("Show in folder")
                onTriggered: root.model.revealScore(card.path)
            }
        }
        OrchestrionToolTip {
            visible: cardMouse.containsMouse && !updateMouse.containsMouse
            text: card.downloaded ? card.nativePath
                                  : qsTr("From Rolls: downloaded when you open it")
        }

        // Rolls has a newer version: replaced only if the user says so, as it
        // may differ from what they practised.
        Rectangle {
            id: updateBadge
            visible: card.updateAvailable && !card.downloading
            anchors.top: slip.top
            anchors.right: slip.right
            anchors.margins: 6
            width: 22
            height: 22
            radius: width / 2
            color: updateMouse.containsMouse && !card.current ? Theme.metalBright
                                                              : Theme.metal
            border.color: Theme.accent
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "↑"
                color: Theme.backdrop
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            MouseArea {
                id: updateMouse
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: true
                // Not the score that is open: it would be saved over the update.
                cursorShape: card.current ? Qt.ArrowCursor : Qt.PointingHandCursor
                onClicked: {
                    if (!card.current)
                        root.model.update(card.path)
                }
            }
            OrchestrionToolTip {
                visible: updateMouse.containsMouse
                text: card.current
                      ? qsTr("A newer version is available: open another score to update this one")
                      : qsTr("A newer version is available: click to update")
            }
        }
    }

    // The way to the user's own scores, at the end of the library until a
    // folder has been added.
    component AddFolderCard: Item {
        id: addCard

        readonly property bool lit: addMouse.containsMouse || activeFocus

        width: flick.rowWidth
        height: 84

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

        Row {
            anchors.centerIn: parent
            spacing: 18

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "+"
                color: outline.ink
                font.pixelSize: 32
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                Text {
                    text: qsTr("Add a folder of your own scores")
                    color: Theme.metalBright
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }
                Text {
                    text: qsTr("Its scores will be listed here, and searched")
                    color: Theme.inkMuted
                    font.pixelSize: 12
                }
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
            id: titleOrnament
            anchors.horizontalCenter: parent.horizontalCenter
            // The rules end where the table's rows do.
            spanWidth: flick.rowWidth
            viewWidth: root.width
            viewHeight: root.height
            title: root.tutorialsOnly ? qsTr("Tutorials") : qsTr("Choose a score")
        }

        // One row when the window is wide enough; wraps when not.
        Flow {
            id: linksRow
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(root.width - 48,
                            searchField.width
                            + (root.tutorialsOnly
                               ? 0
                               : fileLink.width + folderLink.width + onlineLink.width
                                 + 3 * spacing)
                            + (backLink.visible ? backLink.width + spacing : 0))
            spacing: 12
            // blockGap under the title's letters, which end above the
            // ornament's own bottom.
            topPadding: root.blockGap - header.spacing
                        - (titleOrnament.height - titleOrnament.titleBaseline)

            // Fuzzy: the letters in order, not necessarily together ("mzrt"
            // finds Mozart); Return opens the best match.
            TextField {
                id: searchField
                objectName: "demo:Shelf/Search"
                width: 280
                height: 36
                leftPadding: 38
                rightPadding: 32
                placeholderText: root.tutorialsOnly ? qsTr("Search tutorials")
                                                    : qsTr("Search scores")
                // Return chooses, Tab moves on: not the shortcuts'.
                Keys.onShortcutOverride: function(event) {
                    event.accepted = event.key === Qt.Key_Return
                        || event.key === Qt.Key_Enter || event.key === Qt.Key_Tab
                        || event.key === Qt.Key_Backtab
                }
                placeholderTextColor: Theme.inkFaint
                color: Theme.metalBright
                selectionColor: Theme.metal
                selectedTextColor: Theme.backdrop
                font.pixelSize: 14
                verticalAlignment: TextInput.AlignVCenter
                onTextChanged: {
                    root.model.query = text
                    if (root.tutorialsOnly)
                        root.tutorialsQuery = text
                    else
                        root.scoresQuery = text
                }
                Keys.onDownPressed: root.focusFirstCard()
                onAccepted: {
                    if (root.model.openBestMatch(root.tutorialsOnly))
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
                visible: !root.tutorialsOnly
                height: searchField.height
                text: qsTr("Open from computer…")
                onClicked: {
                    root.shown = false
                    root.model.openFromComputer()
                }
            }
            ShelfLink {
                id: folderLink
                visible: !root.tutorialsOnly
                height: searchField.height
                text: qsTr("Add a folder…")
                onClicked: root.model.addFolder()
            }
            ShelfLink {
                id: onlineLink
                visible: !root.tutorialsOnly
                height: searchField.height
                text: qsTr("Search on musescore.com")
                onClicked: root.model.searchOnline()
            }
            // Here, away from the corner: the window's own close button is there.
            // As tall as the row; the drawing is a little wider than tall.
            Item {
                id: backLink
                visible: root.model.hasScore
                height: searchField.height
                width: height * 769 / 728

                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: backTip.text
                Accessible.onPressAction: root.close()
                Keys.onShortcutOverride: function(event) {
                    event.accepted = event.key === Qt.Key_Space
                        || event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                        || event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab
                }
                Keys.onReturnPressed: root.close()
                Keys.onEnterPressed: root.close()
                Keys.onSpacePressed: root.close()
                Keys.onDownPressed: root.focusFirstCard()

                MetalGlyph {
                    anchors.fill: parent
                    source: "qrc:/icons/player/close.svg"
                    hovered: backMouse.containsMouse
                    pressed: backMouse.pressed
                    focused: backLink.activeFocus
                }
                MouseArea {
                    id: backMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
                OrchestrionToolTip {
                    id: backTip
                    visible: backMouse.containsMouse
                    text: qsTr("Back to score")
                }
            }
        }
    }

    // The cards, scrolling on their own under the header.
    Item {
        id: cardsArea
        objectName: "demo:Shelf/Cards"
        anchors.top: header.bottom
        // The headings as far under the search row as the title's letters
        // are above it: blockGap.
        anchors.topMargin: linksRow.y + linksRow.topPadding
                           - (titleOrnament.y + titleOrnament.titleBaseline)
        anchors.bottom: parent.bottom
        // The same air below as the header has above, and a demo's caption
        anchors.bottomMargin: header.anchors.topMargin
                              + (demoCaption.height + header.anchors.topMargin) * root.captionOpen
        anchors.left: parent.left
        anchors.right: parent.right

        readonly property bool empty: (root.tutorialsOnly ? root.model.tutorialCount
                                                          : root.model.shownCount) === 0
                                      && root.model.query.length > 0

        // The columns' headings, over the rows' columns.
        Item {
            id: columnHeadings
            visible: root.showsColumns
            x: (flick.width - flick.rowWidth) / 2
            width: flick.rowWidth
            height: visible ? 8 + titleHeading.implicitHeight + 4
                              + composerHeading.implicitHeight + 8 : 0

            // Framed as the rows are, dents included, but for the slips'
            // column.
            DentedCard {
                x: root.titleX
                width: parent.width - x
                height: parent.height
                dents: [root.opusX - x, root.rhythmX - x]
            }

            // Stacked as in the rows: the title over the composer.
            SortHeading {
                id: titleHeading
                x: root.titleX + root.cellPadding
                anchors.top: parent.top
                anchors.topMargin: 8
                key: "title"
                text: qsTr("Title")
                asTitle: true
            }
            SortHeading {
                id: composerHeading
                x: root.titleX + root.cellPadding
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                key: "composer"
                text: qsTr("Composer")
                tip: qsTr("By surname")
            }
            SortHeading {
                x: root.opusX
                width: root.opusWidth
                anchors.verticalCenter: parent.verticalCenter
                centred: true
                key: "opus"
                text: qsTr("Opus")
            }
            SortHeading {
                id: difficultyHeading
                x: root.rhythmX
                width: parent.width - x
                // On the same line as Title, as Rhythm and Notes are on
                // Composer's.
                anchors.baseline: titleHeading.baseline
                centred: true
                key: "difficulty"
                text: qsTr("Difficulty")
                tip: qsTr("From 1 to 9, for the rhythm and for the notes; the higher of the two sorts. Blank for a score that doesn't say.")
            }
            SortHeading {
                x: root.rhythmX
                width: root.levelWidth
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                centred: true
                key: "rhythm"
                text: qsTr("Rhythm")
            }
            SortHeading {
                x: root.notesX
                width: root.levelWidth
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                centred: true
                key: "notes"
                text: qsTr("Notes")
            }
        }

        Flickable {
            id: flick
            anchors.top: columnHeadings.bottom
            // The cards fade out short of the headings, not against them.
            anchors.topMargin: columnHeadings.visible ? cardsArea.headingsClearance : 0
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            contentWidth: width
            contentHeight: content.height + cardsArea.topFadeHeight + cardsArea.fadeHeight
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

            // One card a row, as wide as the window allows, centred.
            readonly property int columns: 1
            readonly property int rowWidth: Math.min(width - 48, root.maxRowWidth)

            Column {
                id: content
                x: (flick.width - flick.rowWidth) / 2
                // Clear of the fade when scrolled to either end.
                y: cardsArea.topFadeHeight
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
                        readonly property bool isLibrary: kind === "library"
                        readonly property bool isTutorials: kind === "tutorials"
                        readonly property bool showsAddCard: isLibrary
                            && !root.model.hasFolders && !searching

                        // A folder keeps its section while not searching,
                        // even empty: that's where its scores will come.
                        visible: isTutorials === root.tutorialsOnly
                                 && (scores.count > 0 || (isFolder && !searching)
                                     || showsAddCard)
                        width: parent.width
                        spacing: root.sectionLabelSpacing

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
                                // Where the library's scores come from.
                                Text {
                                    visible: shelfSection.isLibrary
                                    anchors.baseline: sectionLabel.baseline
                                    text: shelfSection.subtitle
                                    color: siteMouse.containsMouse ? Theme.metalBright
                                                                   : Theme.inkFaint
                                    font.pixelSize: 12
                                    font.underline: siteMouse.containsMouse

                                    MouseArea {
                                        id: siteMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.model.openRollsSite()
                                    }
                                    OrchestrionToolTip {
                                        visible: siteMouse.containsMouse
                                        text: qsTr("Rolls: free scores, made for Orchestrion")
                                    }
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

                        Column {
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
            y: flick.y + cardsArea.topFadeHeight + 24
            text: (root.tutorialsOnly ? qsTr("No tutorial matches \u201C%1\u201D")
                                      : qsTr("No score matches \u201C%1\u201D"))
                  .arg(searchField.text)
            color: Theme.inkMuted
            font.pixelSize: 15
        }

        // The cards fade in and out at the edges, into the backdrop (opaque,
        // so drawing its colour over them is a fade). The scroll bar's strip
        // is left out, so that it stays crisp.
        readonly property int fadeHeight: 32
        // Shorter at the top, so that the cards start close under the
        // headings: the headings' frame marks that edge already. With the
        // clearance above it, the cards rest as far under the headings.
        readonly property int topFadeHeight: 12
        readonly property int headingsClearance: 8

        Rectangle {
            anchors.top: flick.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 14
            height: cardsArea.topFadeHeight
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
}
