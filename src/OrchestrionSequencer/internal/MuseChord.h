#pragma once

#include "OrchestrionSequencer/IChord.h"

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
class MuseChord : public IChord
{
public:
  MuseChord(const mu::engraving::Segment &segment, int track, int voice,
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
  const int m_track;
  const bool m_isChord;
  const int m_voice;

  const mu::engraving::Segment &m_segment;
};
} // namespace dgk