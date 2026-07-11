declare_module(OrchestrionNotation)

set(MODULE_QRC ${CMAKE_CURRENT_LIST_DIR}/OrchestrionNotation.qrc)

set(MODULE_SRC
  internal/LoopBoundariesController.cpp
  internal/LoopBoundariesController.h
  internal/OrchestrionNotationInteractionProcessor.cpp
  internal/OrchestrionNotationInteractionProcessor.h
  view/HighlightFader.cpp
  view/HighlightFader.h
  view/KineticScroller.cpp
  view/KineticScroller.h
  view/OrchestrionNotationPaintView.cpp
  view/OrchestrionNotationPaintView.h
  ILoopBoundariesController.h
  IOrchestrionNotationInteractionProcessor.h
  OrchestrionNotationModule.cpp
  OrchestrionNotationModule.h
)

set(MODULE_LINK notation)

setup_module()
