/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "MuseOrnament.h"

#include "engraving/dom/chord.h"
#include "engraving/dom/interval.h"
#include "engraving/dom/note.h"
#include "engraving/dom/ornament.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/spanner.h"
#include "engraving/dom/spannermap.h"
#include "engraving/dom/trill.h"
#include "engraving/dom/utils.h"
#include "engraving/types/constants.h"
#include "engraving/types/symid.h"
#include "engraving/types/types.h"

#include <algorithm>
#include <cstdlib>

namespace dgk
{
namespace me = mu::engraving;

namespace
{
/**
 * What an ornament sign spells out: its notes as offsets in ornament
 * intervals (+1 = the auxiliary above, -1 = the one below, 0 = the main
 * note), and what to make of them.
 */
struct Pattern
{
  OrnamentSign sign;
  std::vector<int> offsets;
};

/**
 * Trills, short trills and turns start on the main note: the key the player
 * presses sounds its own note at the press. The rarer compound signs follow
 * MuseScore's realizations, flattened to one pass.
 */
std::optional<Pattern> PatternFor(me::SymId id, me::OrnamentStyle style)
{
  using S = me::SymId;
  const bool baroque = style == me::OrnamentStyle::BAROQUE;
  switch (id)
  {
  case S::ornamentTrill:
  case S::ornamentShake3:
  case S::ornamentShakeMuffat1:
    return Pattern{OrnamentSign::trill,
                   baroque ? std::vector<int>{1, 0} : std::vector<int>{0, 1}};
  case S::ornamentShortTrill:
    return Pattern{OrnamentSign::figure, baroque ? std::vector<int>{1, 0, 1, 0}
                                                 : std::vector<int>{0, 1, 0}};
  case S::ornamentMordent:
  case S::ornamentPinceCouperin:
    return Pattern{OrnamentSign::figure, {0, -1, 0}};
  case S::ornamentTurn:
  case S::ornamentTurnUp:
  case S::ornamentHaydn:
  case S::brassJazzTurn:
    return Pattern{OrnamentSign::figure, {0, 1, 0, -1, 0}};
  case S::ornamentTurnInverted:
  case S::ornamentTurnUpS:
  case S::ornamentTurnSlash:
    return Pattern{OrnamentSign::figure, {0, -1, 0, 1, 0}};
  case S::ornamentTremblement:
  case S::ornamentTremblementCouperin:
    return Pattern{OrnamentSign::figure, {1, 0, 1, 0}};
  case S::ornamentPrallMordent:
    return Pattern{OrnamentSign::figure, {1, 0, -1, 0}};
  case S::ornamentUpPrall:
    return Pattern{OrnamentSign::figure, {-1, 0, 1, 0, 1, 0}};
  case S::ornamentUpMordent:
    return Pattern{OrnamentSign::figure, {-1, 0, 1, 0, -1, 0}};
  case S::ornamentPrecompMordentUpperPrefix:
    return Pattern{OrnamentSign::figure, {1, 1, 1, 0, 1, 0}};
  case S::ornamentDownMordent:
    return Pattern{OrnamentSign::figure, {1, 1, 1, 0, 1, 0, -1, 0}};
  case S::ornamentPrallUp:
    return Pattern{OrnamentSign::figure, {1, 0, 1, 0, -1, 0}};
  case S::ornamentPrallDown:
    return Pattern{OrnamentSign::figure, {1, 0, 1, 0, -1, 0, 0, 0}};
  case S::ornamentLinePrall:
    return Pattern{OrnamentSign::figure, {2, 2, 2, 1, 0, 1, 0}};
  default:
    return std::nullopt;
  }
}

/**
 * The pitch `steps` ornament intervals above (steps > 0) or below (< 0) the
 * note. `ornament` may be null: a trill line without one, at a second.
 */
int AuxiliaryPitch(const me::Note &note, const me::Ornament *ornament,
                   int steps)
{
  const bool above = steps > 0;
  // MuseScore resolves the top note's neighbours itself, with the accidentals
  // of the key and of the bar so far.
  if (ornament && std::abs(steps) == 1 && &note == note.chord()->upNote())
    if (const me::Note *aux =
            above ? ornament->noteAbove() : ornament->noteBelow())
      return aux->pitch();
  const me::OrnamentInterval interval = !ornament
                                            ? me::DEFAULT_ORNAMENT_INTERVAL
                                        : above ? ornament->intervalAbove()
                                                : ornament->intervalBelow();
  int semitones = 0;
  if (interval.type == me::IntervalType::AUTO)
    semitones = std::abs(me::chromaticPitchSteps(
        &note, &note, steps * static_cast<int>(interval.step)));
  else
    semitones = std::abs(steps) *
                me::Interval::fromOrnamentInterval(interval).chromatic;
  return note.pitch() + (above ? semitones : -semitones);
}

std::vector<int> FigureNote(const std::vector<const me::Note *> &notes,
                            const me::Ornament *ornament, int offset)
{
  std::vector<int> pitches;
  pitches.reserve(notes.size());
  for (const me::Note *note : notes)
    pitches.push_back(offset == 0 ? note->pitch()
                                  : AuxiliaryPitch(*note, ornament, offset));
  return pitches;
}

constexpr int thirtySecond = me::Constants::DIVISION / 8;
constexpr int sixtyFourth = me::Constants::DIVISION / 16;

/**
 * The grace chords at `valueTicks` each, or, if that is 0, at their written
 * value.
 */
std::vector<GraceNote> Graces(const std::vector<me::Chord *> &chords,
                              int valueTicks)
{
  std::vector<GraceNote> graces;
  for (const me::Chord *chord : chords)
  {
    GraceNote grace;
    grace.ticks =
        valueTicks > 0 ? valueTicks : chord->durationTypeTicks().ticks();
    for (const me::Note *note : chord->notes())
      if (note->play())
        grace.pitches.push_back(note->pitch());
    if (!grace.pitches.empty())
      graces.push_back(std::move(grace));
  }
  return graces;
}

/**
 * The trill line over the chord, if any — starting on it or covering it.
 */
const me::Trill *TrillLineOver(const me::Chord &chord)
{
  const me::Score *score = chord.score();
  if (!score)
    return nullptr;
  const int tick = chord.tick().ticks();
  for (const auto &interval : score->spannerMap().findOverlapping(tick, tick))
  {
    const me::Spanner *spanner = interval.value;
    if (spanner && spanner->isTrill() && spanner->track() == chord.track() &&
        spanner->tick().ticks() <= tick && tick < spanner->tick2().ticks())
      return me::toTrill(spanner);
  }
  return nullptr;
}

/**
 * The ornament sign on the chord, with the ornament it comes with (for its
 * intervals), if any.
 */
struct Sign
{
  Pattern pattern;
  const me::Ornament *ornament = nullptr;
};

std::optional<Sign> FindSign(const me::Chord &chord)
{
  if (const me::Ornament *ornament = chord.findOrnament())
    if (const auto pattern =
            PatternFor(ornament->symId(), ornament->ornamentStyle()))
      return Sign{*pattern, ornament};
  // A trill line: an imported one carries an ornament without a sign, so go
  // by the line's type; and every chord under the line trills, not only the
  // one it starts on.
  if (const me::Trill *trill = TrillLineOver(chord))
    if (const auto pattern =
            PatternFor(me::Ornament::fromTrillType(trill->trillType()),
                       trill->ornamentStyle()))
      return Sign{*pattern, trill->ornament()};
  return std::nullopt;
}

/**
 * The chord before this one in its voice, or null if a rest, or nothing,
 * comes before.
 */
const me::Chord *PreviousChord(const me::Chord &chord)
{
  for (const me::Segment *segment =
           chord.segment()->prev1(me::SegmentType::ChordRest);
       segment; segment = segment->prev1(me::SegmentType::ChordRest))
    if (const me::EngravingItem *item = segment->element(chord.track()))
      return item->isChord() ? me::toChord(item) : nullptr;
  return nullptr;
}

/**
 * The pitches of the chord's notes that play, sorted.
 */
std::vector<int> PlayedPitches(const me::Chord &chord)
{
  std::vector<int> pitches;
  for (const me::Note *note : chord.notes())
    if (note->play())
      pitches.push_back(note->pitch());
  std::sort(pitches.begin(), pitches.end());
  return pitches;
}

/**
 * Whether the lone grace before the chord is an appoggiatura, leaning on the
 * chord by step — a semitone or a tone away from one of the notes it
 * strikes — rather than a quick grace: one that leaps to the chord, or
 * repeats the note before it (a re-articulation, however it is engraved), is
 * played crushed.
 */
bool IsAppoggiatura(const std::vector<me::Chord *> &graces,
                    const me::Chord &chord,
                    const std::vector<const me::Note *> &struckNotes)
{
  if (graces.size() != 1 ||
      graces.front()->noteType() == me::NoteType::ACCIACCATURA)
    return false;
  const std::vector<int> gracePitches = PlayedPitches(*graces.front());
  if (gracePitches.empty())
    return false;
  if (const me::Chord *previous = PreviousChord(chord);
      previous && PlayedPitches(*previous) == gracePitches)
    return false;
  return std::all_of(gracePitches.begin(), gracePitches.end(),
                     [&](int pitch)
                     {
                       return std::any_of(
                           struckNotes.begin(), struckNotes.end(),
                           [&](const me::Note *note)
                           {
                             const int distance =
                                 std::abs(note->pitch() - pitch);
                             return distance >= 1 && distance <= 2;
                           });
                     });
}

/**
 * Whether the chord resolves a trill: the chord struck before it in its
 * voice, with no rest between, is trilled.
 */
bool ResolvesATrill(const me::Chord &chord)
{
  const me::Chord *previous = PreviousChord(chord);
  // Back to the chord struck, past those merely tied to it.
  while (previous &&
         std::all_of(previous->notes().begin(), previous->notes().end(),
                     [](const me::Note *note)
                     { return note->tieBack() != nullptr; }))
    previous = PreviousChord(*previous);
  if (!previous)
    return false;
  const auto sign = FindSign(*previous);
  return sign && sign->pattern.sign == OrnamentSign::trill;
}
} // namespace

std::optional<Ornament> BuildOrnament(const me::Chord &chord)
{
  std::vector<const me::Note *> notes; // the notes the gesture strikes
  for (const me::Note *note : chord.notes())
    if (!note->tieBack() && note->play())
      notes.push_back(note);
  if (notes.empty())
    return std::nullopt;

  Ornament result;
  const auto sign = FindSign(chord);
  if (sign)
  {
    result.sign = sign->pattern.sign;
    for (const int offset : sign->pattern.offsets)
      result.figure.push_back(FigureNote(notes, sign->ornament, offset));
  }

  // Graces resolving a trill inherit its 32nds; a lone unslashed one leaning
  // on the chord by step is an appoggiatura and keeps its written value; any
  // other — in a run, leaping to the chord, repeating the note before — is a
  // 64th.
  const std::vector<me::Chord *> &gracesBefore = chord.graceNotesBefore(true);
  if (!gracesBefore.empty())
  {
    const bool resolvesATrill = ResolvesATrill(chord);
    result.appoggiatura =
        !resolvesATrill && IsAppoggiatura(gracesBefore, chord, notes);
    result.gracesBefore =
        Graces(gracesBefore, resolvesATrill        ? thirtySecond
                             : result.appoggiatura ? 0
                                                   : sixtyFourth);
  }
  result.gracesAfter =
      Graces(chord.graceNotesAfter(true),
             result.sign == OrnamentSign::trill ? thirtySecond : sixtyFourth);

  if (result.gracesBefore.empty() && result.gracesAfter.empty() &&
      result.sign == OrnamentSign::none)
    return std::nullopt; // a plain chord
  return result;
}
} // namespace dgk
