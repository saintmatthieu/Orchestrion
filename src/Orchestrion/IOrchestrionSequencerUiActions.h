#pragma once

#include "IComputerKeyboard.h"
#include <modularity/imoduleinterface.h>
#include <unordered_map>

namespace dgk
{
class IOrchestrionSequencerUiActions : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrionSequencerUiActions);

public:
  virtual ~IOrchestrionSequencerUiActions() = default;

  virtual std::unordered_map<IComputerKeyboard::Layout, std::string>
  computerKeyboardSetterActionIds() const = 0;
};
} // namespace dgk