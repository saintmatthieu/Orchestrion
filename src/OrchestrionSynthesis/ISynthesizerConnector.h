#pragma once

#include <modularity/ioc.h>

namespace dgk
{
class ISynthesizerConnector : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(ISynthesizerConnector)

public:
  virtual ~ISynthesizerConnector() = default;
  virtual void connectSynthesizer(const muse::audio::AudioResourceMeta &) = 0;
};
} // namespace dgk