set(MODULE_TEST positionestimation_tests)

set(MODULE_TEST_SRC
  ${CMAKE_CURRENT_LIST_DIR}/PositionEstimatorTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/PositionSmootherTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/PositionTrackerTests.cpp
)

set(MODULE_TEST_LINK
  PositionEstimation
)

# gtest.cmake adds the googletest include dir but not googlemock's, which the
# framework's gmain.cpp needs.
set(MODULE_TEST_INCLUDE
  ${MUSE_FRAMEWORK_PATH}/src/framework/testing/thirdparty/googletest/googlemock/include
)

include(${MUSE_FRAMEWORK_PATH}/src/framework/testing/gtest.cmake)
