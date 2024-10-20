set(MODULE OrchestrionNotation)

set(MODULE_QRC ${CMAKE_CURRENT_LIST_DIR}/OrchestrionNotation.qrc)

set(MODULE_SRC
  view/OrchestrionNotationPaintView.cpp
  view/OrchestrionNotationPaintView.h
  OrchestrionNotationModule.cpp
  OrchestrionNotationModule.h
)

set(MODULE_LINK notation)

include(${MuseScore_ROOT_DIR}/build/module.cmake)
