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
#pragma once

#include <QColor>
#include <QString>

#include <ui/uitypes.h>

namespace dgk
{
/**
 * Orchestrion ships two looks — gold on mahogany, silver on slate — and puts
 * them in MuseScore's two standard theme slots: gold is the "light" theme
 * and silver the "dark" one. Neither is light in the usual sense; the
 * slots are just the two the framework persists and hot-reloads for free, and
 * using them means MuseScore's own chrome follows the choice without further
 * plumbing.
 *
 * Both palettes are written out in full in src/App/configs/{light,dark}.cfg
 * under the `orchestrion_` keys. The framework hands every key it does not
 * recognise straight back in ThemeInfo::extra, so those files are the single
 * source of truth for MuseScore's chrome, for the QML palette
 * (OrchestrionTheme, which `Theme` forwards to) and for the C++ painting
 * code alike.
 */
namespace paletteKeys
{
//! "gold" or "silver" — the name the View menu checks its items against.
inline constexpr auto themeName = "orchestrion_theme_name";
//! The title bar and the wallpaper's vignetted edges.
inline constexpr auto backdrop = "orchestrion_backdrop";
//! The parchment plateau the score lies on, and the ink on the backdrop.
inline constexpr auto accent = "orchestrion_accent";
//! Ink for what is drawn *on* an accent-coloured surface.
inline constexpr auto accentInk = "orchestrion_accent_ink";
//! The ornamental metal: rules, diamonds, links, focus rings.
inline constexpr auto metal = "orchestrion_metal";
//! Metal at its brightest: borders and headline figures.
inline constexpr auto metalBright = "orchestrion_metal_bright";
//! Secondary and tertiary ink on the dark surfaces.
inline constexpr auto inkMuted = "orchestrion_ink_muted";
inline constexpr auto inkFaint = "orchestrion_ink_faint";
//! The box behind a ringing note.
inline constexpr auto highlight = "orchestrion_highlight";
//! Near-opaque backings: the score view's floating overlays, and the
//! settings popups (which sit a little more solidly still).
inline constexpr auto overlay = "orchestrion_overlay";
inline constexpr auto popup = "orchestrion_popup";
//! The attribution toast's panel.
inline constexpr auto toast = "orchestrion_toast";
//! File name of the backdrop image, within the installed wallpapers/ folder.
inline constexpr auto wallpaper = "orchestrion_wallpaper";
} // namespace paletteKeys

/**
 * A colour from the theme's `extra` map. Returns an invalid QColor for a
 * missing or unparseable key, which paints as black — visible enough that a
 * typo in a .cfg does not pass unnoticed.
 */
inline QColor paletteColor(const muse::ui::ThemeInfo &theme, const char *key)
{
  return theme.extra.value(QString::fromLatin1(key)).value<QColor>();
}

/**
 * A string from the theme's `extra` map (the theme name, the wallpaper file
 * name). Colour-looking values are stored as QColor, so only keys that cannot
 * parse as a colour come back here.
 */
inline QString paletteString(const muse::ui::ThemeInfo &theme, const char *key)
{
  return theme.extra.value(QString::fromLatin1(key)).toString();
}

//! The theme codes the two palettes live in, by the name they go by.
inline constexpr auto goldThemeName = "gold";
inline constexpr auto silverThemeName = "silver";
} // namespace dgk
