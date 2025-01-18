#pragma once

#include "IChord.h"
#include "MuseChordRestImpl.h"

namespace mu::engraving
{
class Note;
class Segment;
} // namespace mu::engraving

namespace dgk
{
class MuseChord : public IChord, private MuseChordRestImpl
{
public:
  MuseChord(const mu::engraving::Segment &segment, TrackIndex,
            int measurePlaybackTick);

  bool IsChord() const override;
  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;

private:
  std::vector<int> GetPitches() const override;
  std::vector<mu::engraving::Note *> GetNotes() const;
};
} // namespace dgk