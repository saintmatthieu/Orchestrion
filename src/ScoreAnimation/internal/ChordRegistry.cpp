#include "ChordRegistry.h"

namespace dgk
{
void ChordRegistry::RegisterChord(const IChordRest *chord,
                                  const mu::engraving::Segment *segment)
{
  m_chords[chord] = segment;
}

const mu::engraving::Segment *
ChordRegistry::GetSegment(const IChordRest *chord) const
{
  return m_chords.count(chord) ? m_chords.at(chord) : nullptr;
}
} // namespace dgk