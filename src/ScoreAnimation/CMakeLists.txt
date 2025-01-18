set(MODULE ScoreAnimation)

set(MODULE_SRC
  internal/ScoreAnimator.cpp
  internal/ScoreAnimator.h
  internal/SegmentRegistry.cpp
  internal/SegmentRegistry.h
  ISegmentRegistry.h
  IScoreAnimator.h
  ScoreAnimationModule.cpp
  ScoreAnimationModule.h
)

setup_module()
