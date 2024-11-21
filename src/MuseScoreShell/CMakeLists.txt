# We just need an extra library to include what we need from MuseScore's appshell
# yet without getting C2374 redefinition errors

set(MODULE MusescoreShell)

set(MODULE_QML_IMPORT ${MuseScore_ROOT_DIR}/src/appshell/qml)

set(MODULE_SRC
  MusescoreShellModule.cpp
  MusescoreShellModule.h
  view/OrchestrionMenuModel.cpp
  view/OrchestrionMenuModel.h
  OrchestrionActionIds.h
)

set(MODULE_LINK
  appshell
  OrchestrionShell # circular dependency - consider merging the two
  )

setup_module()
