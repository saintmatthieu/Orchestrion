#pragma once

#include "IOrchestrionSequencer.h"
#include "OrchestrionSynthesis/ITrackChannelMapper.h"
#include "OrchestrionTypes.h"
#include "ScoreAnimation/ISegmentRegistry.h"
#include "playback/iplaybackcontroller.h"
#include <memory>
#include <modularity/ioc.h>
#include <vector>

namespace mu::notation
{
class IMasterNotation;
}

namespace dgk
{
class IOrchestrionSequencer;

class OrchestrionSequencerFactory : public muse::Injectable
{
  muse::Inject<ISegmentRegistry> segmentRegistry;
  muse::Inject<ITrackChannelMapper> mapper;

public:
  IOrchestrionSequencerPtr
  CreateSequencer(mu::notation::IMasterNotation &masterNotation);
};
} // namespace dgk