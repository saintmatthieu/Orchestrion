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

#include "OrchestrionSequencer/Performance.h"

#include <QString>
#include <optional>
#include <vector>

namespace dgk
{
/**
 * A recording's notes, transcribed: what the automatic player plays the
 * score as (IOrchestrionSequencer::PlayPerformance).
 *
 * The file is JSON: {"version": 1, "performer": "<name>", "notes": [{"time":
 * <s>, "duration": <s>, "midi": <pitch>, "velocity": <0 to 127>}, ...],
 * "sustain": [{"time": <s>, "value": <0 to 1>}, ...]}, the sustain pedal
 * down from 0.5 and optional, times from where the piece starts in the
 * recording.
 */
QString TranscriptionPath(const QString &userDataPath, const QString &pieceId);

/** None if there is no such file, or not one this version reads. */
std::optional<Performance> ReadTranscription(const QString &path);
} // namespace dgk
