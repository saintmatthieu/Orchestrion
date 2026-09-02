declare_module(PositionEstimation)

set(MODULE_SRC
  PositionEstimator.cpp
  PositionEstimator.h
  PositionSmoother.cpp
  PositionSmoother.h
  PositionTracker.cpp
  PositionTracker.h
)

set(MODULE_USE_PCH OFF)
set(MODULE_USE_UNITY OFF)

setup_module()

if (MUSE_ENABLE_UNIT_TESTS)
    add_subdirectory(tests)
endif()
