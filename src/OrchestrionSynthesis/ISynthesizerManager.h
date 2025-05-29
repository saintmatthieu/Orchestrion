#pragma once

#include "OrchestrionCommon/OrchestrionCommonTypes.h"
#include <async/notification.h>
#include <modularity/imoduleinterface.h>
#include <vector>

namespace dgk
{
class ISynthesizerManager : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(ISynthesizerManager);

public:
  virtual ~ISynthesizerManager() = default;

  virtual std::vector<DeviceDesc> availableSynths() const = 0;
  virtual muse::async::Notification availableSynthsChanged() const = 0;
  virtual bool selectSynth(const std::string &synthId) = 0;
  virtual muse::async::Notification selectedSynthChanged() const = 0;
  virtual std::string selectedSynth() const = 0;
};
} // namespace dgk