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

#include <QString>
#include <optional>

namespace dgk
{
/**
 * Text as fuzzy search compares it: lower case, accents dropped ("Für
 * Elise" and "fur elise" are the same), and anything that isn't a letter or
 * a digit (underscores, hyphens, punctuation) turned into a space.
 */
QString foldForSearch(const QString &text);

/**
 * How well `query` matches `text`, both as returned by foldForSearch(); none
 * if it doesn't. Every word of the query must be found in the text, its
 * letters in order but not necessarily next to each other ("mzrt" finds
 * "mozart"). The score is higher the more of those letters are adjacent and
 * the more of them start words ("ave" is a better match for "ave maria"
 * than for "variation"), so that the best matches can be put first.
 */
std::optional<int> fuzzyScore(const QString &query, const QString &text);
} // namespace dgk
