#pragma once

#include "Orchestrion/OrchestrionTypes.h"
#include <modularity/imoduleinterface.h>

namespace dgk
{
/*!
 * \brief Maps MuseScore tracks (staff index * numVoices + voice index) to
 * MIDI channels.
 */
class ITrackChannelMapper : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(ITrackChannelMapper);

public:
  virtual ~ITrackChannelMapper() = default;

  enum class Policy
  {
    oneChannelPerInstrument,
    oneChannelPerStaff,
  };

  virtual int numChannels(Policy) const = 0;
  virtual int channelForTrack(TrackIndex, Policy) const = 0;
  virtual InstrumentIndex instrumentForStaff(int staff) const = 0;
  virtual std::vector<int> channelsForInstrument(InstrumentIndex,
                                                 Policy) const = 0;
};
} // namespace dgk