/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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

#include "IChord.h"
#include "IModifiableItem.h"
#include "MuseMelodySegment.h"

#include "engraving/types/types.h"

namespace mu::engraving
{
class Note;
class Segment;
} // namespace mu::engraving

namespace dgk
{
class MuseChord : public IChord,
                  public IModifiableItem,
                  private MuseMelodySegment
{
public:
  MuseChord(const mu::engraving::Segment &segment, TrackIndex,
            int measurePlaybackTick);

  const IChord *AsChord() const override;
  IChord *AsChord() override;

  const IRest *AsRest() const override;
  IRest *AsRest() override;

  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;

private:
  std::vector<int> GetPitches() const override;
  std::vector<mu::engraving::Note *> GetNotes() const;
  void SetModified();

  // IChord
private:
  float GetVelocity() const override;
  void SetVelocity(float) override;

  // IModifiableItem
private:
  bool Modified() const override;
  void Save() override;
  void RevertChanges() override;

  std::optional<mu::engraving::Color> m_originalColor;
  std::optional<float> m_unsavedVelocity;
};
} // namespace dgk