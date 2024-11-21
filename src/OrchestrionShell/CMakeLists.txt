set(MODULE OrchestrionShell)

set(MODULE_QRC ${CMAKE_CURRENT_LIST_DIR}/OrchestrionShell.qrc)

set(MODULE_SRC
  internal/OrchestrionUiActions.cpp
  internal/OrchestrionUiActions.h
  view/NotationPaintViewLoaderModel.cpp
  view/NotationPaintViewLoaderModel.h
  IOrchestrionUiActions.h
  OrchestrionShellModule.cpp
  OrchestrionShellModule.h
)

set(MODULE_LINK MusescoreShell)

setup_module()
