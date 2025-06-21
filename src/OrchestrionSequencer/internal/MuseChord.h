#pragma once

#include "IChord.h"
#include "MuseMelodySegment.h"

namespace mu::engraving
{
class Note;
class Segment;
} // namespace mu::engraving

namespace dgk
{
class MuseChord : public IChord, private MuseMelodySegment
{
public:
  MuseChord(const mu::engraving::Segment &segment, TrackIndex,
            int measurePlaybackTick);

  const IChord *AsChord() const override;
  IChord *AsChord() override;

  const IRest *AsRest() const override;
  IRest *AsRest() override;

  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;

private:
  std::vector<int> GetPitches() const override;
  std::vector<mu::engraving::Note *> GetNotes() const;
};
} // namespace dgk