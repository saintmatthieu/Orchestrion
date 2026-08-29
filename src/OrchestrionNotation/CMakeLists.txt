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
  view/ReadingFocus.cpp
  view/ReadingFocus.h
  ILoopBoundariesController.h
  view/ScoreFollower.cpp
  view/ScoreFollower.h
  view/TempoVisualizationView.cpp
  view/TempoVisualizationView.h
  view/TempoVizModel.cpp
  view/TempoVizModel.h
  view/TimingFeedbackOverlay.cpp
  view/TimingFeedbackOverlay.h
  IOrchestrionNotationInteractionProcessor.h
  OrchestrionNotationModule.cpp
  OrchestrionNotationModule.h
)

set(MODULE_LINK notation PositionEstimation)

setup_module()
