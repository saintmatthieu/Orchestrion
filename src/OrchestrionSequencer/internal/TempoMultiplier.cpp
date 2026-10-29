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
#include "TempoMultiplier.h"

#include <engraving/dom/masterscore.h>
#include <engraving/dom/tempotimeline.h>
#include <engraving/editing/editscoreproperties.h>
#include <notation/imasternotation.h>
#include <notation/inotation.h>
#include <notation/inotationplayback.h>
#include <notation/inotationundostack.h>

#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
/** The meta tag, and in it the multiplier as a decimal number: "0.85". */
const muse::String tagName = u"orchestrion-tempo-multiplier";
} // namespace

void TempoMultiplier::init()
{
  const auto watchNotation = [this]
  {
    if (const auto master = globalContext()->currentMasterNotation())
      // Edits, undos and redos all go through the undo stack.
      master->notation()->undoStack()->stackChanged().onNotify(
          this, [this] { checkChanged(); }, Mode::SetReplace);
    checkChanged();
  };
  globalContext()->currentMasterNotationChanged().onNotify(this, watchNotation);
  watchNotation();
}

double TempoMultiplier::savedMultiplier() const
{
  const auto master = globalContext()->currentMasterNotation();
  if (!master || !master->masterScore())
    return 1.0;
  bool ok = false;
  const double value =
      master->masterScore()->metaTag(tagName).toDouble(&ok);
  return ok && value > 0.0 ? std::clamp(value, min, max) : 1.0;
}

double TempoMultiplier::multiplier() const { return savedMultiplier(); }

void TempoMultiplier::setMultiplier(double value)
{
  const auto master = globalContext()->currentMasterNotation();
  if (!master || !master->masterScore() || !(value > 0.0))
    return;
  // Three decimals are more than any tempo readout tells apart, and keep the
  // tag readable.
  value = std::round(std::clamp(value, min, max) * 1000.0) / 1000.0;
  if (value == savedMultiplier())
    return;

  mu::engraving::MasterScore *const score = master->masterScore();
  std::map<muse::String, muse::String> tags = score->metaTags();
  if (value == 1.0)
    tags.erase(tagName);
  else
    tags[tagName] = muse::String::number(value);
  const auto undoStack = master->notation()->undoStack();
  undoStack->prepareChanges(
      muse::TranslatableString("undoableAction", "Change tempo"));
  score->undo(new mu::engraving::ChangeMetaTags(score, tags));
  undoStack->commitChanges();
}

muse::async::Notification TempoMultiplier::multiplierChanged() const
{
  return m_multiplierChanged;
}

void TempoMultiplier::checkChanged()
{
  const double value = savedMultiplier();
  if (const auto master = globalContext()->currentMasterNotation();
      master && master->masterScore() &&
      master->masterScore()->tempoTimeline().tempoMultiplier().val != value)
    master->playback()->setTempoMultiplier(value);
  if (value == m_multiplier)
    return;
  m_multiplier = value;
  m_multiplierChanged.notify();
}
} // namespace dgk
