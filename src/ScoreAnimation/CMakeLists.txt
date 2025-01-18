set(MODULE ScoreAnimation)

set(MODULE_SRC
  internal/ChordRegistry.cpp
  internal/ChordRegistry.h
  internal/ScoreAnimator.cpp
  internal/ScoreAnimator.h
  IChordRestRegistry.h
  IScoreAnimator.h
  ScoreAnimationModule.cpp
  ScoreAnimationModule.h
)

setup_module()
