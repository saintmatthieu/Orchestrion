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
#include "TrackChannelMapper.h"

namespace dgk
{
// For now something that assumes there is only one instrument in the score, and
// that it has two staves.
// TODO: implement the logic, ie. probably get the information of how what
// instruments and staves there are in the score and which instrument is
// selected for performance.

int TrackChannelMapper::numChannels(Policy policy) const
{
  return policy == Policy::oneChannelPerInstrument ? 1 : 2;
}

int TrackChannelMapper::channelForTrack(TrackIndex track, Policy policy) const
{
  return policy == Policy::oneChannelPerInstrument ? 0 : track.staffIndex();
}

InstrumentIndex TrackChannelMapper::instrumentForStaff(int) const
{
  return InstrumentIndex{0};
}

std::vector<int> TrackChannelMapper::channelsForInstrument(InstrumentIndex,
                                                           Policy policy) const
{
  return policy == Policy::oneChannelPerInstrument ? std::vector<int>{0}
                                                   : std::vector<int>{0, 1};
}

} // namespace dgk