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

namespace dgk
{
/**
 * Run from an AppImage, which registers nothing by itself: makes the desktop
 * know it, by a .desktop file in the user's applications, so that
 * orchestrion:// links reach it and MusicXML files offer it. Written again
 * whenever the AppImage has moved. It takes the links, which are only its own,
 * but no file type from the user's default application. Does nothing when not
 * run from an AppImage.
 */
void registerAppImageWithDesktop();
} // namespace dgk
