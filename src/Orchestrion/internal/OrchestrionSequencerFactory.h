#pragma once

#include "OrchestrionSynthesis/ITrackChannelMapper.h"
#include "OrchestrionTypes.h"
#include "ScoreAnimation/IChordRegistry.h"
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
  muse::Inject<IChordRegistry> chordRegistry;
  muse::Inject<ITrackChannelMapper> mapper;

public:
  std::unique_ptr<IOrchestrionSequencer> CreateSequencer(
      mu::notation::IMasterNotation &masterNotation,
      const mu::playback::IPlaybackController::InstrumentTrackIdMap &);
};
} // namespace dgk