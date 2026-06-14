declare_module(OrchestrionNotation)

set(MODULE_QRC ${CMAKE_CURRENT_LIST_DIR}/OrchestrionNotation.qrc)

set(MODULE_SRC
  internal/OrchestrionNotationInteractionProcessor.cpp
  internal/OrchestrionNotationInteractionProcessor.h
  view/HighlightFader.cpp
  view/HighlightFader.h
  view/KineticScroller.cpp
  view/KineticScroller.h
  view/OrchestrionNotationPaintView.cpp
  view/OrchestrionNotationPaintView.h
  view/TempoFollower.cpp
  view/TempoFollower.h
  IOrchestrionNotationInteractionProcessor.h
  OrchestrionNotationModule.cpp
  OrchestrionNotationModule.h
)

set(MODULE_LINK notation TempoTracking)

setup_module()
