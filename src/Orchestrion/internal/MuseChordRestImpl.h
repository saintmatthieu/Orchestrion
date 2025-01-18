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
class MuseChordRestImpl
{
protected:
  MuseChordRestImpl(const mu::engraving::Segment &segment, TrackIndex,
                    int measurePlaybackTick);

  Tick GetBeginTick() const;

  const Tick m_tick;
  const TrackIndex m_track;
  const mu::engraving::Segment &m_segment;
};
} // namespace dgk