set(MODULE_TEST orchestrionsequencer_tests)

# The scheduler, the performance follower and what it follows are pure (no
# Qt, no MuseScore): compiled straight into the test rather than linking the
# whole module.
set(MODULE_TEST_SRC
  ${CMAKE_CURRENT_LIST_DIR}/OrnamentScheduleTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/OrnamentSchedule.cpp
  ${CMAKE_CURRENT_LIST_DIR}/PerformanceFollowerTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/PerformanceFollower.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ScoreEventsTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/ScoreEvents.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/OrnamentGesture.cpp
  ${CMAKE_CURRENT_LIST_DIR}/TiedVoicesTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/TiedVoices.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/SplitChord.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/VoiceBlank.cpp
)

set(MODULE_TEST_LINK
)

# gtest.cmake adds the googletest include dir but not googlemock's, which the
# framework's gmain.cpp needs. googletest comes from muse_deps, which records
# its source dir in this property.
get_property(_gtest_src GLOBAL PROPERTY googletest_SOURCE_DIR)
set(MODULE_TEST_INCLUDE
  ${CMAKE_CURRENT_LIST_DIR}/..
  ${_gtest_src}/googletest/googlemock/include
)

include(${MUSE_FRAMEWORK_SRC_PATH}/testing/gtest.cmake)
