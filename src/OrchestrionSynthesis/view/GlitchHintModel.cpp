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
#include "GlitchHintModel.h"
#include "MuseScoreShell/OrchestrionActionIds.h"

namespace dgk
{
GlitchHintModel::GlitchHintModel(QObject *parent) : QObject{parent}
{
  if (outputLatency())
    outputLatency()->glitchHintChanged().onNotify(this, [this]
                                                  { emit shownChanged(); });
}

bool GlitchHintModel::shown() const
{
  return outputLatency() && outputLatency()->glitchHintShown();
}

void GlitchHintModel::openSettings()
{
  close();
  if (dispatcher())
    dispatcher()->dispatch(actionIds::showLatency);
}

void GlitchHintModel::close()
{
  if (outputLatency())
    outputLatency()->closeGlitchHint(false);
}

void GlitchHintModel::closeForever()
{
  if (outputLatency())
    outputLatency()->closeGlitchHint(true);
}
} // namespace dgk
