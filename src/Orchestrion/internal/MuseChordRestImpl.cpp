#include "MuseChordRestImpl.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tie.h"
#include <cassert>

namespace dgk
{

namespace me = mu::engraving;

MuseChordRestImpl::MuseChordRestImpl(const me::Segment &segment,
                                     TrackIndex track, int measurePlaybackTick)
    : m_tick{measurePlaybackTick + segment.rtick().ticks(),
             segment.tick().ticks()},
      m_track{track}, m_segment{segment}
{
}

dgk::Tick MuseChordRestImpl::GetBeginTick() const { return m_tick; }
} // namespace dgk