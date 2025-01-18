#include "MuseRest.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tie.h"
#include <cassert>

namespace dgk
{

namespace me = mu::engraving;

MuseRest::MuseRest(const me::Segment &segment, TrackIndex track,
                   int measurePlaybackTick)
    : MuseChordRestImpl(segment, track, measurePlaybackTick)
{
}

const IChord *MuseRest::AsChord() const { return nullptr; }
IChord *MuseRest::AsChord() { return nullptr; }

const IRest *MuseRest::AsRest() const { return this; }
IRest *MuseRest::AsRest() { return this; }

dgk::Tick MuseRest::GetBeginTick() const
{
  return MuseChordRestImpl::GetBeginTick();
}

dgk::Tick MuseRest::GetEndTick() const
{
  const me::Segment *segment = &m_segment;
  auto endTick = GetBeginTick();
  while (segment)
  {
    const auto rest =
        dynamic_cast<const me::Rest *>(segment->element(m_track.value));
    if (!rest)
      break;
    endTick += rest->actualTicks().ticks();
    constexpr auto includeOtherVoicesOnStaff = false;
    segment = segment->nextCR(m_track.value, includeOtherVoicesOnStaff);
  }
  return endTick;
}
} // namespace dgk