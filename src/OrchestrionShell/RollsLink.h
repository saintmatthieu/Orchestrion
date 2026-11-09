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

#include <QRegularExpression>
#include <QString>
#include <optional>

namespace dgk
{
/** Whether it can be the id of a score of Rolls, a slug which names a file. */
inline bool isRollsId(const QString &id)
{
  static const QRegularExpression pattern{QRegularExpression::anchoredPattern(
      QStringLiteral("[a-z0-9]+(-[a-z0-9]+)*"))};
  return id.size() <= 128 && pattern.match(id).hasMatch();
}

/** Whether it is one of Orchestrion's links, valid or not, and not a file. */
inline bool isOrchestrionLink(const QString &argument)
{
  return argument.startsWith(QStringLiteral("orchestrion:"),
                             Qt::CaseInsensitive);
}

/**
 * The id in orchestrion://rolls/<id>, the only link there is. Web pages can
 * make these: only an id is taken from them, never an address or a path, so
 * that the score downloaded is always one the Rolls index lists.
 */
inline std::optional<QString> rollsIdOfLink(const QString &link)
{
  const QString prefix = QStringLiteral("orchestrion://rolls/");
  if (!link.startsWith(prefix))
    return std::nullopt;
  QString id = link.mid(prefix.size());
  // Some browsers add one.
  if (id.endsWith('/'))
    id.chop(1);
  if (!isRollsId(id))
    return std::nullopt;
  return id;
}
} // namespace dgk
