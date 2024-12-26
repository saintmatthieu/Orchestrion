#include "ChordRegistry.h"

namespace dgk::orchestrion
{
void ChordRegistry::RegisterChord(const IChord *chord,
                                  const mu::engraving::Segment *segment)
{
  m_chords[chord] = segment;
}

const mu::engraving::Segment *
ChordRegistry::GetSegment(const IChord *chord) const
{
  return m_chords.count(chord) ? m_chords.at(chord) : nullptr;
}
} // namespace dgk::orchestrion