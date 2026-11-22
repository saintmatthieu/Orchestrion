set(MODULE_TEST orchestrionshell_tests)

set(MODULE_TEST_SRC
  ${CMAKE_CURRENT_LIST_DIR}/RecordingSyncSessionTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/RecordingSyncSession.cpp
  ${CMAKE_CURRENT_LIST_DIR}/VideoClockTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/VideoClock.cpp
)

set(MODULE_TEST_LINK
)

get_property(_gtest_src GLOBAL PROPERTY googletest_SOURCE_DIR)
set(MODULE_TEST_INCLUDE
  ${CMAKE_CURRENT_LIST_DIR}/..
  ${CMAKE_CURRENT_LIST_DIR}/../..
  ${_gtest_src}/googletest/googlemock/include
)

include(${MUSE_FRAMEWORK_SRC_PATH}/testing/gtest.cmake)
