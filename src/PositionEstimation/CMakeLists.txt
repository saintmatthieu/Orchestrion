declare_module(PositionEstimation)

set(MODULE_SRC
  PositionEstimator.cpp
  PositionEstimator.h
  PositionSmoother.cpp
  PositionSmoother.h
  PositionTracker.cpp
  PositionTracker.h
)

set(MODULE_NOT_LINK_GLOBAL ON)
set(MODULE_USE_PCH_NONE ON)
set(MODULE_USE_UNITY_NONE ON)

setup_module()

if (MUSE_ENABLE_UNIT_TESTS)
    add_subdirectory(tests)
endif()
