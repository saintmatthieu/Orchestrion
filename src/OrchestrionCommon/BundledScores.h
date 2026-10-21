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

#include <QCollator>
#include <QDir>
#include <QFileInfoList>
#include <algorithm>

namespace dgk
{
/**
 * The scores shipped with the application (the `scores/` folder in its
 * data), sorted as people would: ignoring case and with numbers by value,
 * so that "Variation 10" comes after "Variation 9".
 */
inline QFileInfoList bundledScores(const QString &scoresDir)
{
  QFileInfoList scores = QDir{scoresDir}.entryInfoList(
      {"*.mscz", "*.mscx", "*.mxl", "*.musicxml", "*.xml"}, QDir::Files);
  QCollator collator;
  collator.setNumericMode(true);
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  std::sort(scores.begin(), scores.end(),
            [&](const QFileInfo &a, const QFileInfo &b) {
              return collator.compare(a.completeBaseName(),
                                      b.completeBaseName()) < 0;
            });
  return scores;
}
} // namespace dgk
