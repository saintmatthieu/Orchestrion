set(MODULE_TEST tempotracking_tests)

set(MODULE_TEST_SRC
  ${CMAKE_CURRENT_LIST_DIR}/TempoSmootherTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/TempoTrackerTests.cpp
)

set(MODULE_TEST_LINK
  TempoTracking
)

# gtest.cmake adds the googletest include dir but not googlemock's, which the
# framework's gmain.cpp needs.
set(MODULE_TEST_INCLUDE
  ${MUSE_FRAMEWORK_PATH}/src/framework/testing/thirdparty/googletest/googlemock/include
)

include(${MUSE_FRAMEWORK_PATH}/src/framework/testing/gtest.cmake)
