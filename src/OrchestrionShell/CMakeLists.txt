declare_module(OrchestrionShell)

set(MODULE_QRC ${CMAKE_CURRENT_LIST_DIR}/OrchestrionShell.qrc)

set(MODULE_SRC
  internal/ControllerMenuManager.cpp
  internal/ControllerMenuManager.h
  internal/DeviceMenuManager.cpp
  internal/DeviceMenuManager.h
  internal/OrchestrionActionController.cpp
  internal/OrchestrionActionController.h
  internal/OrchestrionEventProcessor.cpp
  internal/OrchestrionEventProcessor.h
  internal/OrchestrionUiActions.cpp
  internal/OrchestrionUiActions.h
  internal/OrchestrionStartupScenario.cpp
  internal/OrchestrionStartupScenario.h
  internal/PlaybackDeviceMenuManager.cpp
  internal/PlaybackDeviceMenuManager.h
  internal/SynthesizerMenuManager.cpp
  internal/SynthesizerMenuManager.h
  view/NotationPaintViewLoaderModel.cpp
  view/NotationPaintViewLoaderModel.h
  view/OrchestrionWindowTitleProvider.cpp
  view/OrchestrionWindowTitleProvider.h
  IOrchestrionStartupScenario.h
  IOrchestrionUiActions.h
  OrchestrionShellModule.cpp
  OrchestrionShellModule.h
  OrchestrionShellTypes.h
)

set(MODULE_INCLUDE
  ${vst3sdk_SOURCE_DIR}
)

setup_module()
