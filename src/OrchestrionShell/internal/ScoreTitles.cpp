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
#include "ScoreTitles.h"
#include "MuseScorePlaceholderMetaTags.h"

#include <QStringList>
#include <algorithm>
#include <iterator>

namespace dgk
{
namespace
{
/**
 * Whether `text` is one of MuseScore's new-score placeholder texts (in any
 * language), i.e. a meta tag the author never actually filled in.
 */
template <size_t N>
bool isPlaceholder(const QString &text,
                   const char16_t *const (&placeholders)[N])
{
  return std::any_of(std::begin(placeholders), std::end(placeholders),
                     [&](const char16_t *placeholder)
                     {
                       return text.compare(QString::fromUtf16(placeholder),
                                           Qt::CaseInsensitive) == 0;
                     });
}
} // namespace

QString titleFromFileName(const muse::io::path_t &path)
{
  return muse::io::completeBasename(path)
      .toQString()
      .replace('_', ' ')
      .simplified();
}

QString scoreTitle(const mu::project::ProjectMeta &meta,
                   const muse::io::path_t &path)
{
  QString title = meta.title.simplified();
  if (isPlaceholder(title, musescore_placeholders::titles))
    title.clear();
  if (title.isEmpty())
    title = titleFromFileName(path);
  return title;
}

QString scoreComposer(const mu::project::ProjectMeta &meta)
{
  // Multi-line tags ("Music: X\nArrangement: Y") go on the one line that
  // headings offer, separated rather than run together.
  QStringList lines;
  for (const QString &line : meta.composer.split('\n'))
    if (const QString simplified = line.simplified();
        !simplified.isEmpty() &&
        !isPlaceholder(simplified, musescore_placeholders::composers))
      lines << simplified;
  return lines.join(QStringLiteral(" · "));
}
} // namespace dgk
