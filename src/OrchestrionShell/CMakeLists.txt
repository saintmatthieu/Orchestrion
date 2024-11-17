set(MODULE OrchestrionShell)

set(MODULE_QRC ${CMAKE_CURRENT_LIST_DIR}/OrchestrionShell.qrc)

set(MODULE_SRC
  internal/OrchestrionActionController.cpp
  internal/OrchestrionActionController.h
  view/NotationPaintViewLoaderModel.cpp
  view/NotationPaintViewLoaderModel.h
  OrchestrionShellModule.cpp
  OrchestrionShellModule.h
  IOrchestrionActionController.h
)

set(MODULE_LINK MusescoreShell)

setup_module()
