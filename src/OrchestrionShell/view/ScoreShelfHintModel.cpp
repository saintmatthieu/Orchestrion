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
#include "ScoreShelfHintModel.h"

#include <global/settings.h>

namespace dgk
{
namespace
{
const muse::Settings::Key
    SCORE_SHELF_HINT_DISMISSED("OrchestrionShell",
                               "SCORE_SHELF_HINT_DISMISSED");
} // namespace

ScoreShelfHintModel::ScoreShelfHintModel(QObject *parent) : QObject(parent) {}

void ScoreShelfHintModel::load()
{
  muse::settings()->setDefaultValue(SCORE_SHELF_HINT_DISMISSED,
                                    muse::Val{false});
  m_dismissed = muse::settings()->value(SCORE_SHELF_HINT_DISMISSED).toBool();
  if (m_dismissed)
    return;
  m_end.reachedEnd().onNotify(this,
                              [this]
                              {
                                if (!m_dismissed)
                                  setShown(true);
                              });
  m_end.leftEnd().onNotify(this, [this] { setShown(false); });
  m_end.start();
}

void ScoreShelfHintModel::dismiss()
{
  setShown(false);
  if (m_dismissed)
    return;
  m_dismissed = true;
  muse::settings()->setSharedValue(SCORE_SHELF_HINT_DISMISSED, muse::Val{true});
}

bool ScoreShelfHintModel::shown() const { return m_shown; }

void ScoreShelfHintModel::setShown(bool shown)
{
  if (shown == m_shown)
    return;
  m_shown = shown;
  emit shownChanged();
}
} // namespace dgk
