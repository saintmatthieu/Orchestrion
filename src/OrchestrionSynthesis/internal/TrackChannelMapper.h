#pragma once

#include "ITrackChannelMapper.h"

namespace dgk
{
class TrackChannelMapper : public ITrackChannelMapper
{
public:
  TrackChannelMapper() = default;

private:
  int numChannels(Policy) const override;
  int channelForTrack(TrackIndex, Policy) const override;
  InstrumentIndex instrumentForStaff(int staff) const override;
  std::vector<int> channelsForInstrument(InstrumentIndex instrument,
                                         Policy) const override;
};
} // namespace dgk