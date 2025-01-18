#pragma once

#include "OrchestrionSynthesis/ITrackChannelMapper.h"
#include "OrchestrionTypes.h"
#include "ScoreAnimation/IChordRestRegistry.h"
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
  muse::Inject<IChordRestRegistry> chordRegistry;
  muse::Inject<ITrackChannelMapper> mapper;

public:
  std::unique_ptr<IOrchestrionSequencer>
  CreateSequencer(mu::notation::IMasterNotation &masterNotation);
};
} // namespace dgk