#pragma once

#include <audio/audiotypes.h>
#include <modularity/ioc.h>

namespace dgk
{
class ISynthesizerConnector : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(ISynthesizerConnector)

public:
  virtual ~ISynthesizerConnector() = default;
  virtual void connectVstInstrument(const muse::audio::AudioResourceId &) = 0;
  virtual void connectFluidSynth() = 0;
  virtual void disconnect() = 0;
};
} // namespace dgk