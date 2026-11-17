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
// A moment to take in the end of the piece before the cards come up.
constexpr auto delayAfterEnd = std::chrono::seconds{1};

const muse::Settings::Key
    SCORE_SHELF_HINT_DISMISSED("OrchestrionShell",
                               "SCORE_SHELF_HINT_DISMISSED");
} // namespace

ScoreShelfHintModel::ScoreShelfHintModel(QObject *parent)
    : QObject(parent), m_end{delayAfterEnd}
{
}

void ScoreShelfHintModel::load()
{
  muse::settings()->setDefaultValue(SCORE_SHELF_HINT_DISMISSED,
                                    muse::Val{false});
  m_shelfKnown = muse::settings()->value(SCORE_SHELF_HINT_DISMISSED).toBool();
  if (m_shelfKnown)
    emit shelfKnownChanged();
  m_end.reachedEnd().onNotify(this, [this] { setShown(true); });
  m_end.leftEnd().onNotify(this, [this] { setShown(false); });
  m_end.start();
}

void ScoreShelfHintModel::dismiss()
{
  m_end.cancel();
  setShown(false);
  if (m_shelfKnown)
    return;
  m_shelfKnown = true;
  muse::settings()->setSharedValue(SCORE_SHELF_HINT_DISMISSED, muse::Val{true});
  emit shelfKnownChanged();
}

void ScoreShelfHintModel::hide()
{
  m_end.cancel();
  setShown(false);
}

bool ScoreShelfHintModel::shown() const { return m_shown; }

bool ScoreShelfHintModel::shelfKnown() const { return m_shelfKnown; }

void ScoreShelfHintModel::setShown(bool shown)
{
  if (shown == m_shown)
    return;
  m_shown = shown;
  emit shownChanged();
}
} // namespace dgk
