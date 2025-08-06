declare_module(OrchestrionNotation)

set(MODULE_QRC ${CMAKE_CURRENT_LIST_DIR}/OrchestrionNotation.qrc)

set(MODULE_SRC
  internal/OrchestrionNotationInteractionProcessor.cpp
  internal/OrchestrionNotationInteractionProcessor.h
  view/OrchestrionNotationPaintView.cpp
  view/OrchestrionNotationPaintView.h
  IOrchestrionNotationInteractionProcessor.h
  OrchestrionNotationModule.cpp
  OrchestrionNotationModule.h
)

set(MODULE_LINK notation)

setup_module()
