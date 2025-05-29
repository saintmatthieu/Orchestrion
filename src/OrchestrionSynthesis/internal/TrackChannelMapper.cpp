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