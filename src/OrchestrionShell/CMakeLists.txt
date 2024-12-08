set(MODULE OrchestrionShell)

set(MODULE_QRC ${CMAKE_CURRENT_LIST_DIR}/OrchestrionShell.qrc)

set(MODULE_SRC
  internal/DeviceMenuManager.cpp
  internal/DeviceMenuManager.h
  internal/MidiControllerMenuManager.cpp
  internal/MidiControllerMenuManager.h
  internal/OrchestrionUiActions.cpp
  internal/OrchestrionUiActions.h
  internal/OrchestrionStartupScenario.cpp
  internal/OrchestrionStartupScenario.h
  internal/PlaybackDeviceMenuManager.cpp
  internal/PlaybackDeviceMenuManager.h
  view/NotationPaintViewLoaderModel.cpp
  view/NotationPaintViewLoaderModel.h
  view/OrchestrionWindowTitleProvider.cpp
  view/OrchestrionWindowTitleProvider.h
  IMidiControllerManager.h
  IOrchestrionStartupScenario.h
  IOrchestrionUiActions.h
  IPlaybackDeviceManager.h
  OrchestrionShellModule.cpp
  OrchestrionShellModule.h
  OrchestrionShellTypes.h
)

setup_module()
