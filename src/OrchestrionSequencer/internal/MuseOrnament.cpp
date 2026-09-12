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
#include "engraving/dom/utils.h"
#include "engraving/types/constants.h"
#include "engraving/types/symid.h"
#include "engraving/types/types.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace dgk
{
namespace me = mu::engraving;

namespace
{
constexpr int semiquaver = me::Constants::DIVISION / 4;     // 120 ticks
constexpr int demisemiquaver = me::Constants::DIVISION / 8; // 60 ticks

/**
 * How an ornament sign is realized, after MuseScore's playback rules
 * (OrnamentsRenderer's disclosure patterns): pitch offsets in ornament
 * intervals (+1 = the auxiliary above, -1 = the one below, 0 = the
 * principal), and the sub-note value, in ticks, by tempo class of the score.
 */
struct Pattern
{
  std::vector<int> prefix;
  std::vector<int> body;
  bool repeatBody = false;
  std::vector<int> suffix;
  struct
  {
    float slow;
    float moderate;
    float fast;
  } subNoteTicks;
};

std::optional<Pattern> PatternFor(me::SymId id, me::OrnamentStyle style)
{
  using S = me::SymId;
  constexpr float d = demisemiquaver;
  constexpr float s = semiquaver;
  const bool baroque = style == me::OrnamentStyle::BAROQUE;
  switch (id)
  {
  case S::ornamentTrill:
  case S::ornamentShake3:
  case S::ornamentShakeMuffat1:
    return baroque ? Pattern{{}, {1, 0}, true, {-1, 0}, {d * 0.8f, d, s}}
                   : Pattern{{}, {0, 1}, true, {}, {d * 0.8f, d, s}};
  case S::ornamentLinePrall:
    return Pattern{{2, 2, 2}, {1, 0}, true, {1, 0}, {d, d, d}};
  case S::ornamentUpPrall:
    return Pattern{{-1, 0}, {1, 0}, true, {1, 0}, {d, d, d}};
  case S::ornamentUpMordent:
    return Pattern{{-1, 0}, {1, 0}, true, {-1, 0}, {s, s, s}};
  case S::ornamentShortTrill:
    return baroque ? Pattern{{1, 0, 1}, {0}, false, {}, {d / 2, d, s}}
                   : Pattern{{0, 1}, {0}, false, {}, {d / 2, d, s}};
  case S::ornamentMordent:
  case S::ornamentPinceCouperin:
    return Pattern{{0, -1}, {0}, false, {}, {d / 2, d, s}};
  case S::ornamentPrecompMordentUpperPrefix:
    return Pattern{{1, 1, 1, 0}, {1, 0}, true, {}, {s, s, s}};
  case S::ornamentDownMordent:
    return Pattern{{1, 1, 1, 0}, {1, 0}, true, {-1, 0}, {s, s, s}};
  case S::ornamentPrallUp:
    return Pattern{{1, 0}, {1, 0}, true, {-1, 0}, {s, s, s}};
  case S::ornamentPrallDown:
    return Pattern{{1, 0}, {1, 0}, true, {-1, 0, 0, 0}, {s, s, s}};
  case S::ornamentTurn:
  case S::ornamentTurnUp:
  case S::ornamentHaydn:
  case S::brassJazzTurn:
    return Pattern{{1, 0, -1}, {0}, false, {}, {d / 2, d, s}};
  case S::ornamentTurnInverted:
  case S::ornamentTurnUpS:
  case S::ornamentTurnSlash:
    return Pattern{{-1, 0, 1}, {0}, false, {}, {d / 2, d, s}};
  case S::ornamentTremblement:
  case S::ornamentTremblementCouperin:
    return Pattern{{1, 0}, {1, 0}, false, {}, {s, s, s}};
  case S::ornamentPrallMordent:
    return Pattern{{}, {1, 0, -1, 0}, false, {}, {s, s, s}};
  default:
    return std::nullopt;
  }
}

/**
 * The sub-note value at the score's tempo: MuseScore's tempo classes
 * (moderato from 108 bpm, prestissimo from 200).
 */
float SubNoteTicks(const Pattern &pattern, double bps)
{
  if (bps >= 200.0 / 60.0)
    return pattern.subNoteTicks.fast;
  if (bps >= 108.0 / 60.0)
    return pattern.subNoteTicks.moderate;
  return pattern.subNoteTicks.slow;
}

/**
 * The pitch `steps` ornament intervals above (steps > 0) or below (< 0) the
 * note.
 */
int AuxiliaryPitch(const me::Note &note, const me::Ornament &ornament,
                   int steps)
{
  const bool above = steps > 0;
  // MuseScore resolves the top note's neighbours itself, with the accidentals
  // of the key and of the bar so far.
  if (std::abs(steps) == 1 && &note == note.chord()->upNote())
    if (const me::Note *aux =
            above ? ornament.noteAbove() : ornament.noteBelow())
      return aux->pitch();
  const me::OrnamentInterval interval =
      above ? ornament.intervalAbove() : ornament.intervalBelow();
  int semitones = 0;
  if (interval.type == me::IntervalType::AUTO)
    semitones = std::abs(me::chromaticPitchSteps(
        &note, &note, steps * static_cast<int>(interval.step)));
  else
    semitones = std::abs(steps) *
                me::Interval::fromOrnamentInterval(interval).chromatic;
  return note.pitch() + (above ? semitones : -semitones);
}

OrnamentStep Step(const std::vector<const me::Note *> &notes,
                  const me::Ornament *ornament, int offset, int ticks)
{
  OrnamentStep step;
  step.ticks = ticks;
  step.pitches.reserve(notes.size());
  for (const me::Note *note : notes)
    step.pitches.push_back(offset == 0 || !ornament
                               ? note->pitch()
                               : AuxiliaryPitch(*note, *ornament, offset));
  return step;
}

int TotalTicks(const std::vector<OrnamentStep> &steps)
{
  int total = 0;
  for (const OrnamentStep &step : steps)
    total += step.ticks;
  return total;
}

/**
 * The grace chords as steps, their written durations squeezed to fit
 * `availableTicks` if they don't.
 */
void AppendGraceSteps(std::vector<OrnamentStep> &steps,
                      const std::vector<me::Chord *> &graces,
                      int availableTicks)
{
  int written = 0;
  for (const me::Chord *grace : graces)
    written += grace->durationTypeTicks().ticks();
  if (written <= 0)
    return;
  const double scale = std::min(1.0, availableTicks / double(written));
  for (const me::Chord *grace : graces)
  {
    OrnamentStep step;
    step.ticks = static_cast<int>(
        std::lround(grace->durationTypeTicks().ticks() * scale));
    for (const me::Note *note : grace->notes())
      if (note->play())
        step.pitches.push_back(note->pitch());
    if (!step.pitches.empty() && step.ticks > 0)
      steps.push_back(std::move(step));
  }
}
} // namespace

std::optional<Ornament> BuildOrnament(const me::Chord &chord, int nominalTicks,
                                      double nominalBps)
{
  std::vector<const me::Note *> notes; // the notes the gesture strikes
  for (const me::Note *note : chord.notes())
    if (!note->tieBack() && note->play())
      notes.push_back(note);
  if (notes.empty() || nominalTicks <= 0)
    return std::nullopt;

  Ornament result;
  // What the grace notes leave the chord itself.
  int remaining = nominalTicks;

  // Grace notes before the chord: MuseScore gives a lone appoggiatura half the
  // chord, and acciaccaturas (or several graces) a sixty-fourth each at most,
  // within that half.
  const std::vector<me::Chord *> &gracesBefore = chord.graceNotesBefore(true);
  if (!gracesBefore.empty())
  {
    const int half = nominalTicks / 2;
    const bool appoggiatura =
        gracesBefore.size() == 1 &&
        gracesBefore.front()->noteType() != me::NoteType::ACCIACCATURA;
    const int available = appoggiatura
                              ? half
                              : std::min(static_cast<int>(gracesBefore.size()) *
                                             demisemiquaver / 2,
                                         half);
    AppendGraceSteps(result.before, gracesBefore, available);
    remaining -= TotalTicks(result.before);
  }

  // Grace notes after the chord, at its end: half the chord at most.
  std::vector<OrnamentStep> gracesAfter;
  AppendGraceSteps(gracesAfter, chord.graceNotesAfter(true), nominalTicks / 2);
  remaining -= TotalTicks(gracesAfter);

  bool ornamented = false;
  if (const me::Ornament *ornament = chord.findOrnament())
    if (const auto pattern =
            PatternFor(ornament->symId(), ornament->ornamentStyle()))
    {
      const float sub = SubNoteTicks(*pattern, nominalBps);
      const int subTicks = static_cast<int>(std::lround(sub));
      const int prefixTicks =
          static_cast<int>(std::lround(pattern->prefix.size() * sub));
      const int suffixTicks =
          static_cast<int>(std::lround(pattern->suffix.size() * sub));
      const int bodyTicks =
          pattern->repeatBody
              ? static_cast<int>(std::lround(pattern->body.size() * sub))
              : 1;
      // Too short a note to ornament (MuseScore's rule), or no room left for
      // the body once its opening and closing notes are placed.
      if (remaining > pattern->subNoteTicks.slow &&
          remaining >= prefixTicks + suffixTicks + bodyTicks)
      {
        for (const int offset : pattern->prefix)
          result.before.push_back(Step(notes, ornament, offset, subTicks));
        for (const int offset : pattern->body)
          result.fill.push_back(Step(notes, ornament, offset, subTicks));
        result.cycleFill = pattern->repeatBody;
        for (const int offset : pattern->suffix)
          result.after.push_back(Step(notes, ornament, offset, subTicks));
        ornamented = true;
      }
    }

  if (!ornamented)
  {
    if (result.before.empty() && gracesAfter.empty())
      return std::nullopt; // a plain chord
    result.fill.push_back(Step(notes, nullptr, 0, std::max(remaining, 1)));
  }

  result.after.insert(result.after.end(), gracesAfter.begin(),
                      gracesAfter.end());
  return result;
}
} // namespace dgk
