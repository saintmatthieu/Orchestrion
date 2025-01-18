#pragma once

#include "IRest.h"
#include "MuseMelodySegment.h"

namespace mu::engraving
{
class Note;
class Segment;
} // namespace mu::engraving

namespace dgk
{
class MuseRest : public IRest, private MuseMelodySegment
{
public:
  MuseRest(const mu::engraving::Segment &segment, TrackIndex,
           int measurePlaybackTick);

  const IChord *AsChord() const override;
  IChord *AsChord() override;

  const IRest *AsRest() const override;
  IRest *AsRest() override;

  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;
};
} // namespace dgk