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
pragma Singleton
import QtQuick 2.15

import Orchestrion.OrchestrionShell 1.0

// Orchestrion's palette, in whichever of the two themes is current — gold on
// mahogany, or silver on slate (View ▸ Theme). The colours themselves live in
// src/App/configs/{light,dark}.cfg; OrchestrionTheme reads them from there and
// re-notifies on a switch, so binding to these properties is enough to follow
// the theme. See src/OrchestrionCommon/OrchestrionPalette.h.
QtObject {
    // "gold" or "silver", for the rare case of picking an asset rather than a
    // colour (the pedal icons, which carry their metal baked in).
    readonly property string name: OrchestrionTheme.name

    // The parchment plateau the score lies on, and the ink of everything drawn
    // on the dark chrome.
    readonly property color accent: OrchestrionTheme.accent
    // Ink for what is drawn *on* an accent-coloured surface.
    readonly property color accentInk: OrchestrionTheme.accentInk
    // The uniform color of the wallpaper's vignetted edges; the title bar is
    // painted with it so it blends into the backdrop.
    readonly property color backdrop: OrchestrionTheme.backdrop
    // The ornamental metal: rules, diamonds, links, focus rings. Bright enough
    // on the backdrop to read as a glint rather than a flat line.
    readonly property color metal: OrchestrionTheme.metal
    // Metal at its brightest: borders and headline figures.
    readonly property color metalBright: OrchestrionTheme.metalBright
    // Secondary and tertiary ink on the dark surfaces.
    readonly property color inkMuted: OrchestrionTheme.inkMuted
    readonly property color inkFaint: OrchestrionTheme.inkFaint
    // Near-opaque backings: the score view's floating overlays, and the
    // settings popups (which sit a little more solidly still).
    readonly property color overlay: OrchestrionTheme.overlay
    readonly property color popup: OrchestrionTheme.popup
    // The attribution toast's panel.
    readonly property color toast: OrchestrionTheme.toast
}
