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

#include "OrchestrionSequencer/IMelodySegment.h"

#include <modularity/imoduleinterface.h>

namespace mu
{
namespace engraving
{
class Segment;
}
} // namespace mu

namespace dgk
{
class ISegmentRegistry : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(ISegmentRegistry);

public:
  virtual ~ISegmentRegistry() = default;

  virtual void RegisterSegment(IMelodySegmentWPtr,
                               const mu::engraving::Segment *) = 0;
  virtual void UnregisterSegment(const IMelodySegment *) = 0;
  virtual const mu::engraving::Segment *
  GetSegment(const IMelodySegment *) const = 0;
  virtual std::vector<IMelodySegment *> GetMelodySegments() = 0;
};
} // namespace dgk