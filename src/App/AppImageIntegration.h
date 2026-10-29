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
 * Run from an AppImage, which installs nothing: makes the desktop know
 * Orchestrion, so that `orchestrion://` links open it and MusicXML files can
 * be opened with it (it doesn't become their default application). Its
 * desktop entry and icon are written to the user's data directory, again if
 * the AppImage moved. Does nothing otherwise.
 */
void integrateAppImage();
} // namespace dgk
