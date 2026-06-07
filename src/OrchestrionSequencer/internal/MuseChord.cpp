/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "MuseChord.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/dynamic.h"
#include "engraving/dom/hairpin.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/spanner.h"
#include "engraving/dom/spannermap.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/tie.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>

namespace dgk
{
namespace me = mu::engraving;

MuseChord::MuseChord(const me::Segment &segment, TrackIndex track,
                     int measurePlaybackTick)
    : MuseMelodySegment(segment, track, measurePlaybackTick)
{
}

std::vector<me::Note *> MuseChord::GetNotes() const
{
  if (const auto museChord =
          dynamic_cast<const me::Chord *>(m_segment.element(m_track.value)))
    return museChord->notes();
  return {};
}

const IChord *MuseChord::AsChord() const { return this; }

IChord *MuseChord::AsChord() { return this; }

const IRest *MuseChord::AsRest() const { return nullptr; }

IRest *MuseChord::AsRest() { return nullptr; }

std::vector<int> MuseChord::GetPitches() const
{
  std::vector<int> chord;
  const auto notes = GetNotes();
  for (const auto note : notes)
    if (!note->tieBack())
      chord.push_back(note->pitch());
  return chord;
}

dgk::Tick MuseChord::GetBeginTick() const
{
  return MuseMelodySegment::GetBeginTick();
}

namespace
{
// Copied and adapted from mu::engraving::Chord::nextTiedChord() ;
// that version compaes whether the tuplet() of the next chord is the same as
// the current chord; I don't know why, but for our purpose this causes a
// problem.
const mu::engraving::Chord *GetNextTiedChord(const mu::engraving::Chord &chord)
{
  using namespace mu::engraving;

  constexpr auto includeOtherVoicesOnStaff = false;
  const Segment *nextSeg =
      chord.segment()->nextCR(chord.track(), includeOtherVoicesOnStaff);
  if (!nextSeg)
    return nullptr;

  auto chordRest =
      dynamic_cast<const ChordRest *>(nextSeg->element(chord.track()));
  assert(chordRest);
  if (!chordRest || !chordRest->isChord())
    return nullptr;

  const Chord *next = toChord(nextSeg->element(chord.track()));

  // If there are more notes in the coming chord, then it can't be a tied chord.
  if (chord.notes().size() < next->notes().size())
    return nullptr;

  // saintmatthieu: `sameSize` is also an argument of the original
  // `Chord::nextTiedChord()`. In our case I think it doesn't matter if the next
  // chord has less notes ; we can do as though they were implicit.
  constexpr auto sameSize = false;
  if (sameSize && chord.notes().size() != next->notes().size())
    return nullptr; // sizes don't match so some notes can't be tied

  // saintmatthieu: this is the part causing the problem.
  // if (tuplet() != next->tuplet())
  // {
  //   return nullptr; // next chord belongs to a different tuplet
  // }

  for (const Note *n : chord.notes())
  {
    const Tie *tie = n->tieFor();
    if (!tie)
      return nullptr; // not tied
    const Note *nn = tie->endNote();
    if (!nn || nn->chord() != next)
      return nullptr; // tied to note in wrong voice, or tied over rest
  }
  return next; // all notes in this chord are tied to notes in next chord
}
} // namespace

dgk::Tick MuseChord::GetEndTick() const
{
  auto chord =
      dynamic_cast<const me::Chord *>(m_segment.element(m_track.value));
  auto endTick = GetBeginTick();
  while (chord)
  {
    endTick += chord->ticks().ticks();
    chord = GetNextTiedChord(*chord);
  }
  return endTick;
}

float MuseChord::GetVelocity() const
{
  if (m_unsavedVelocity.has_value())
    return *m_unsavedVelocity;
  const auto notes = GetNotes();
  if (notes.empty())
    return 0.f;
  return static_cast<float>(notes.front()->userVelocity()) / 127.f;
}

namespace
{
// The "nuance" dynamics that set a sustained playback level (pppppp … ffffff),
// as opposed to momentary accents (sf, fp, …) which we don't treat as a level.
bool IsOrdinaryDynamic(me::DynamicType type)
{
  return type >= me::DynamicType::PPPPPP && type <= me::DynamicType::FFFFFF;
}

// Whether a dynamic / hairpin (both expose dynRange(), staffIdx() and part())
// applies to the given staff/part of the chord we're voicing.
template <typename T>
bool AppliesTo(const T &item, me::staff_idx_t staffIdx, const me::Part *part)
{
  switch (item.dynRange())
  {
  case me::DynamicRange::STAFF:
    return item.staffIdx() == staffIdx;
  case me::DynamicRange::PART:
  case me::DynamicRange::SYSTEM:
    return item.part() == part;
  }
  return false;
}

// The most recent ordinary dynamic at or before `chordSeg` that applies to this
// staff/part. The MIDI velocity comes from MuseScore's standard
// dynamics-to-velocity table (Dynamic::velocity(), e.g. p=49, mf=80, f=96).
std::optional<int> NominalDynamic(const me::Segment &chordSeg,
                                  me::staff_idx_t staffIdx, const me::Part *part)
{
  for (const me::Segment *seg = &chordSeg; seg; seg = seg->prev1())
  {
    const me::Dynamic *applicable = nullptr;
    for (me::EngravingItem *annotation : seg->annotations())
    {
      if (!annotation || !annotation->isDynamic())
        continue;
      const auto dynamic = me::toDynamic(annotation);
      if (dynamic->playDynamic() && IsOrdinaryDynamic(dynamic->dynamicType()) &&
          AppliesTo(*dynamic, staffIdx, part))
        applicable = dynamic;
    }
    if (applicable)
      return applicable->velocity();
  }
  return std::nullopt;
}

// If a crescendo/diminuendo "swell" covers this chord, return the MIDI velocity
// interpolated along the hairpin between the level it starts from (`levelFrom`)
// and the level it leads to; std::nullopt if no hairpin applies here.
std::optional<int> SwellVelocity(const me::Segment &chordSeg,
                                 me::staff_idx_t staffIdx, const me::Part *part,
                                 int levelFrom)
{
  const auto score = chordSeg.score();
  const int chordTick = chordSeg.tick().ticks();

  const me::Hairpin *hairpin = nullptr;
  for (const auto &interval :
       score->spannerMap().findOverlapping(chordTick, chordTick))
  {
    const auto spanner = interval.value;
    if (!spanner || !spanner->isHairpin() || !spanner->playSpanner())
      continue;
    const auto candidate = me::toHairpin(spanner);
    if (AppliesTo(*candidate, staffIdx, part))
    {
      hairpin = candidate;
      break;
    }
  }
  if (!hairpin)
    return std::nullopt;

  const int from = hairpin->tick().ticks();
  const int to = from + std::abs(hairpin->ticks().ticks());
  // A chord exactly at the hairpin's end already gets the target dynamic
  // through the discrete path, so only interpolate strictly inside.
  if (chordTick < from || chordTick >= to)
    return std::nullopt;

  const bool crescendo = hairpin->isCrescendo();

  // The level the hairpin leads to: the ordinary dynamic placed at (or before)
  // its end tick, if it agrees with the hairpin's direction.
  std::optional<int> levelTo;
  for (const me::Segment *seg = chordSeg.next1();
       seg && seg->tick().ticks() <= to; seg = seg->next1())
    for (me::EngravingItem *annotation : seg->annotations())
    {
      if (!annotation || !annotation->isDynamic())
        continue;
      const auto dynamic = me::toDynamic(annotation);
      if (dynamic->playDynamic() && IsOrdinaryDynamic(dynamic->dynamicType()) &&
          AppliesTo(*dynamic, staffIdx, part))
        levelTo = dynamic->velocity(); // keep the latest (closest to the end)
    }

  // No usable end dynamic (or one that contradicts the hairpin direction):
  // swell by a single dynamic "notch", as MuseScore does for open hairpins.
  constexpr int kOpenHairpinStep = 16;
  if (!levelTo || (crescendo ? *levelTo <= levelFrom : *levelTo >= levelFrom))
    levelTo = std::clamp(
        levelFrom + (crescendo ? kOpenHairpinStep : -kOpenHairpinStep), 0, 127);

  const float fraction =
      static_cast<float>(chordTick - from) / static_cast<float>(to - from);
  return static_cast<int>(levelFrom + fraction * (*levelTo - levelFrom) + 0.5f);
}
} // namespace

std::optional<float> MuseChord::GetDynamicVelocity() const
{
  const auto score = m_segment.score();
  if (!score)
    return std::nullopt;

  const auto staffIdx = me::track2staff(m_track.value);
  const auto staff = score->staff(staffIdx);
  const auto part = staff ? staff->part() : nullptr;

  const auto nominal = NominalDynamic(m_segment, staffIdx, part);
  if (!nominal)
    // No dynamic context at all; the caller falls back to its neutral default.
    return std::nullopt;

  if (const auto swell = SwellVelocity(m_segment, staffIdx, part, *nominal))
    return static_cast<float>(*swell) / 127.f;

  return static_cast<float>(*nominal) / 127.f;
}

void MuseChord::SetVelocity(float velocity)
{
  const auto wasModified = m_unsavedVelocity.has_value();
  m_unsavedVelocity = velocity;
  if (!wasModified)
    SetModified();
}

void MuseChord::Save()
{
  const auto notes = GetNotes();
  if (notes.empty())
    return;

  ResetNoteColors(notes);

  if (m_unsavedVelocity.has_value())
  {
    const auto midiVelocity = static_cast<int>(*m_unsavedVelocity * 127);
    for (const auto note : notes)
      note->setUserVelocity(midiVelocity);
    m_unsavedVelocity.reset();
  }
}

void MuseChord::SetModified()
{
  const auto notes = GetNotes();
  if (notes.empty())
    return;
  // Dark violet
  constexpr auto modifiedRgb = "#B040B0";
  for (auto note : notes)
    note->setColor(modifiedRgb);
  m_modifiedChanged.notify();
}

bool MuseChord::Modified() const { return m_unsavedVelocity.has_value(); }

void MuseChord::RevertChanges()
{
  if (!Modified())
    return;
  ResetNoteColors(GetNotes());
  m_unsavedVelocity.reset();
}

void MuseChord::ResetNoteColors(const std::vector<mu::engraving::Note *> &notes)
{
  for (auto note : notes)
    // This assumes the default color is black. It was true when I debugged
    // default note colors, not sure how future-proof this is.
    note->setColor("black");
}

muse::async::Notification MuseChord::ModifiedChanged()
{
  return m_modifiedChanged;
}
} // namespace dgk