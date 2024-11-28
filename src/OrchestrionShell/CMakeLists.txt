set(MODULE OrchestrionShell)

set(MODULE_QRC ${CMAKE_CURRENT_LIST_DIR}/OrchestrionShell.qrc)

set(MODULE_SRC
  internal/DeviceMenuManager.cpp
  internal/DeviceMenuManager.h
  internal/MidiControllerMenuManager.cpp
  internal/MidiControllerMenuManager.h
  internal/OrchestrionUiActions.cpp
  internal/OrchestrionUiActions.h
  internal/PlaybackDeviceMenuManager.cpp
  internal/PlaybackDeviceMenuManager.h
  view/NotationPaintViewLoaderModel.cpp
  view/NotationPaintViewLoaderModel.h
  view/OrchestrionWindowTitleProvider.cpp
  view/OrchestrionWindowTitleProvider.h
  IOrchestrionUiActions.h
  OrchestrionShellModule.cpp
  OrchestrionShellModule.h
  OrchestrionShellTypes.h
)

set(MODULE_LINK MusescoreShell)

setup_module()
