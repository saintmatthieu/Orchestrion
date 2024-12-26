#include "MuseChord.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tie.h"
#include <cassert>

namespace dgk
{

namespace me = mu::engraving;

MuseChord::MuseChord(const me::Segment &segment, int track, int voice,
                     int measurePlaybackTick)
    : m_tick{measurePlaybackTick + segment.rtick().ticks(),
             segment.tick().ticks()},
      m_track{track},
      m_isChord{dynamic_cast<const me::Chord *>(segment.element(m_track)) !=
                nullptr},
      m_voice{voice}, m_segment{segment}
{
}

std::vector<me::Note *> MuseChord::GetNotes() const
{
  if (const auto museChord =
          dynamic_cast<const me::Chord *>(m_segment.element(m_track)))
    return museChord->notes();
  return {};
}

bool MuseChord::IsChord() const { return m_isChord; }

std::vector<int> MuseChord::GetPitches() const
{
  std::vector<int> chord;
  const auto notes = GetNotes();
  for (const auto note : notes)
    if (!note->tieBack())
      chord.push_back(note->pitch());
  return chord;
}

dgk::Tick MuseChord::GetBeginTick() const { return m_tick; }

dgk::Tick MuseChord::GetEndTick() const
{
  if (m_isChord)
    return GetChordEndTick();
  else
    return GetRestEndTick();
}

dgk::Tick MuseChord::GetChordEndTick() const
{
  auto chord = dynamic_cast<const me::Chord *>(m_segment.element(m_track));
  auto endTick = GetBeginTick();
  while (chord)
  {
    endTick += chord->actualTicks().ticks();
    chord = chord->nextTiedChord();
  }
  return endTick;
}

dgk::Tick MuseChord::GetRestEndTick() const
{
  const me::Segment *segment = &m_segment;
  auto endTick = GetBeginTick();
  while (segment)
  {
    const auto rest = dynamic_cast<const me::Rest *>(segment->element(m_track));
    if (!rest)
      break;
    endTick += rest->actualTicks().ticks();
    segment = segment->next1();
  }
  return endTick;
}
} // namespace dgk