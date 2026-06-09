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

// An explicit dynamic marking with the tick it sits at.
struct DynamicAnchor
{
  int tick;     // the dynamic's tick
  int velocity; // MIDI 0..127
};

// The most recent ordinary dynamic at or before `chordSeg` that applies to this
// staff/part. The MIDI velocity comes from MuseScore's standard
// dynamics-to-velocity table (Dynamic::velocity(), e.g. p=49, mf=80, f=96).
std::optional<DynamicAnchor> NominalDynamic(const me::Segment &chordSeg,
                                            me::staff_idx_t staffIdx,
                                            const me::Part *part)
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
      return DynamicAnchor{seg->tick().ticks(), applicable->velocity()};
  }
  return std::nullopt;
}

// The ordinary dynamic an active hairpin leads to: the latest one placed after
// this chord and at or before the hairpin's end `toTick`, if any. Used to aim a
// crescendo/diminuendo at its written target (e.g. the f in p < f).
std::optional<int> EndDynamic(const me::Segment &chordSeg,
                              me::staff_idx_t staffIdx, const me::Part *part,
                              int toTick)
{
  std::optional<int> levelTo;
  for (const me::Segment *seg = chordSeg.next1();
       seg && seg->tick().ticks() <= toTick; seg = seg->next1())
    for (me::EngravingItem *annotation : seg->annotations())
    {
      if (!annotation || !annotation->isDynamic())
        continue;
      const auto dynamic = me::toDynamic(annotation);
      if (dynamic->playDynamic() && IsOrdinaryDynamic(dynamic->dynamicType()) &&
          AppliesTo(*dynamic, staffIdx, part))
        levelTo = dynamic->velocity(); // keep the latest (closest to the end)
    }
  return levelTo;
}

// The playback velocity (0..1) implied by the score's dynamic markings at this
// chord, or 0 if none applies. The score is fully built by the time a MuseChord
// is constructed and the chord's position is fixed, so this never changes — it
// is computed once and cached.
float ComputeDynamicVelocity(const me::Segment &segment, TrackIndex track)
{
  const auto score = segment.score();
  if (!score)
    return 0.f;

  const auto staffIdx = me::track2staff(track.value);
  const auto staff = score->staff(staffIdx);
  const auto part = staff ? staff->part() : nullptr;

  const auto anchor = NominalDynamic(segment, staffIdx, part);
  if (!anchor)
    // No dynamic context at all; the caller falls back to its neutral default.
    return 0.f;

  const int chordTick = segment.tick().ticks();

  // Crescendo/diminuendo hairpins that start at or after the anchor dynamic and
  // reach up to this chord, in start order. A hairpin starting before the
  // anchor is superseded by it. We replay them so that the level a swell
  // reaches *persists* past its end, instead of snapping back to the anchor.
  std::vector<const me::Hairpin *> hairpins;
  for (const auto &interval :
       score->spannerMap().findOverlapping(anchor->tick, chordTick))
  {
    const auto spanner = interval.value;
    if (!spanner || !spanner->isHairpin() || !spanner->playSpanner())
      continue;
    const auto hairpin = me::toHairpin(spanner);
    if (hairpin->tick().ticks() >= anchor->tick &&
        AppliesTo(*hairpin, staffIdx, part))
      hairpins.push_back(hairpin);
  }
  std::sort(hairpins.begin(), hairpins.end(),
            [](const me::Hairpin *a, const me::Hairpin *b)
            { return a->tick().ticks() < b->tick().ticks(); });

  // Each open hairpin (no written terminal dynamic) shifts the running level by
  // one dynamic "notch", as MuseScore does, and that shift persists afterwards.
  constexpr int kOpenHairpinStep = 16;
  int level = anchor->velocity;
  for (const me::Hairpin *hairpin : hairpins)
  {
    const int from = hairpin->tick().ticks();
    const int to = from + std::abs(hairpin->ticks().ticks());
    if (to <= from)
      continue;
    const bool crescendo = hairpin->isCrescendo();
    const int openTarget = std::clamp(
        level + (crescendo ? kOpenHairpinStep : -kOpenHairpinStep), 0, 127);

    if (chordTick >= to)
    {
      // Completed before this chord: persist its end level. (A hairpin with a
      // written terminal dynamic surfaces that dynamic as the anchor, so any
      // hairpin reaching here is open.)
      level = openTarget;
    }
    else if (chordTick >= from)
    {
      // Active over this chord: interpolate toward its terminal dynamic if it
      // agrees with the hairpin's direction, else toward the open target.
      const auto endLevel = EndDynamic(segment, staffIdx, part, to);
      const int target =
          endLevel && (crescendo ? *endLevel > level : *endLevel < level)
              ? *endLevel
              : openTarget;
      const float fraction =
          static_cast<float>(chordTick - from) / static_cast<float>(to - from);
      level = static_cast<int>(level + fraction * (target - level) + 0.5f);
      break; // the active hairpin determines the final level
    }
    // Hairpin entirely after this chord: nothing to apply.
  }

  return static_cast<float>(level) / 127.f;
}
} // namespace

MuseChord::MuseChord(const me::Segment &segment, TrackIndex track,
                     int measurePlaybackTick)
    : MuseMelodySegment{segment, track, measurePlaybackTick},
      m_dynamicVelocity{ComputeDynamicVelocity(segment, track)}
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

std::optional<float> MuseChord::GetDynamicVelocity() const
{
  if (m_dynamicVelocity > 0.f)
    return m_dynamicVelocity;
  return std::nullopt;
}

const me::Chord *MuseChord::GetEngravingChord() const
{
  return dynamic_cast<const me::Chord *>(m_segment.element(m_track.value));
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