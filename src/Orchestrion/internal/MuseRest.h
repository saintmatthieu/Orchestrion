#pragma once

#include "IRest.h"
#include "MuseChordRestImpl.h"

namespace mu::engraving
{
class Note;
class Segment;
} // namespace mu::engraving

namespace dgk
{
class MuseRest : public IRest, private MuseChordRestImpl
{
public:
  MuseRest(const mu::engraving::Segment &segment, TrackIndex,
           int measurePlaybackTick);

  bool IsChord() const override;
  std::vector<int> GetPitches() const override { return {}; }
  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;
};
} // namespace dgk