#pragma once

#include "Orchestrion/IChordRest.h"

namespace mu::engraving
{
class Note;
class Segment;
} // namespace mu::engraving

namespace mu::notation
{
class INotationInteraction;
} // namespace mu::notation

namespace dgk
{
class MuseChordRest : public IChordRest
{
public:
  MuseChordRest(const mu::engraving::Segment &segment, TrackIndex,
                int measurePlaybackTick);

  bool IsChord() const override;
  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;

private:
  std::vector<int> GetPitches() const override;
  Tick GetChordEndTick() const;
  Tick GetRestEndTick() const;

  std::vector<mu::engraving::Note *> GetNotes() const;

  const Tick m_tick;
  const TrackIndex m_track;
  const bool m_isChord;

  const mu::engraving::Segment &m_segment;
};
} // namespace dgk