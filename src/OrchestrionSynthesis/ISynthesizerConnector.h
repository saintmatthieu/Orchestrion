#pragma once

#include <modularity/ioc.h>

namespace muse::audio
{
struct AudioResourceMeta;
}

namespace dgk::orchestrion
{
class ISynthesizerConnector : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(ISynthesizerConnector)

public:
  virtual ~ISynthesizerConnector() = default;
  virtual void connectSynthesizer(const muse::audio::AudioResourceMeta &) = 0;
};
} // namespace dgk::orchestrion