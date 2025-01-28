#include "SegmentRegistry.h"

namespace dgk
{
void SegmentRegistry::RegisterSegment(const IMelodySegment *chord,
                                      const mu::engraving::Segment *segment)
{
  m_chords[chord] = segment;
}

void SegmentRegistry::UnregisterSegment(const IMelodySegment *chord)
{
  m_chords.erase(chord);
}

const mu::engraving::Segment *
SegmentRegistry::GetSegment(const IMelodySegment *chord) const
{
  return m_chords.count(chord) ? m_chords.at(chord) : nullptr;
}
} // namespace dgk