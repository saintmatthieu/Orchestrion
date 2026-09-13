set(MODULE_TEST orchestrionsequencer_tests)

# The scheduler is pure (no Qt, no MuseScore): compiled straight into the
# test rather than linking the whole module.
set(MODULE_TEST_SRC
  ${CMAKE_CURRENT_LIST_DIR}/OrnamentScheduleTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../internal/OrnamentSchedule.cpp
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
