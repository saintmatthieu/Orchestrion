set(MODULE OrchestrionShell)

set(MODULE_QRC ${MuseScore_ROOT_DIR}/src/appshell/appshell.qrc)
set(MODULE_QML_IMPORT ${MuseScore_ROOT_DIR}/src/appshell/qml)

set(MODULE_SRC
  internal/OrchestrionActionController.cpp
  internal/OrchestrionActionController.h
  view/OrchestrionMenuModel.cpp
  view/OrchestrionMenuModel.h
  OrchestrionShellModule.cpp
  OrchestrionShellModule.h
  IOrchestrionActionController.h
)

include(${MuseScore_ROOT_DIR}/build/module.cmake)
