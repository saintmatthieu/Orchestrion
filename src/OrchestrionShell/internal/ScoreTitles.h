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

#include <io/path.h>
#include <project/types/projectmeta.h>

#include <QString>

namespace dgk
{
/**
 * A score's file name made presentable, for when the score has no title of
 * its own: no extension, and the underscores that stand for spaces in
 * downloaded scores turned back into spaces.
 */
QString titleFromFileName(const muse::io::path_t &path);

/**
 * The work's title as the score names it (its "workTitle" meta tag, unless
 * that is one of MuseScore's new-score placeholders), else the file name
 * made presentable. May be empty.
 */
QString scoreTitle(const mu::project::ProjectMeta &meta,
                   const muse::io::path_t &path);

/**
 * The score's "composer" meta tag on one line (a multi-line tag's lines
 * separated by a middle dot), placeholders left out. May be empty.
 */
QString scoreComposer(const mu::project::ProjectMeta &meta);
} // namespace dgk
