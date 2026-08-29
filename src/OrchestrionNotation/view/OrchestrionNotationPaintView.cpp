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
#include "OrchestrionNotationPaintView.h"
#include "OrchestrionSequencer/IChord.h"
#include "OrchestrionSequencer/IOrchestrionSequencer.h"
#include "OrchestrionSequencer/IRest.h"
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QQuickWindow>
#include <QTimer>
#include <QVariantMap>
#include <QWheelEvent>
#include <algorithm>
#include <engraving/dom/chord.h>
#include <engraving/dom/masterscore.h>
#include <engraving/dom/measure.h>
#include <engraving/dom/note.h>
#include <engraving/dom/repeatlist.h>
#include <engraving/dom/segment.h>
#include <engraving/dom/system.h>
#include <engraving/dom/tie.h>
#include <engraving/types/constants.h>
#include <notation/imasternotation.h>

#include <cmath>
#include <optional>

namespace dgk
{
namespace
{
// No dynamics verdict until the loudness curve holds this many gestures: a
// curve through two or three points fits the performer by construction (the
// same reasoning as the timing judgments' window).
constexpr std::size_t dynamicsMinKnots = 4;

// The sequencer routes input events to hands by pitch (< 60 = left hand, see
// OrchestrionSequencer::OnInputEventRecursive) — the same sentinels the
// automatic player uses.
constexpr int rightHandPitch = 60;
constexpr int leftHandPitch = 59;

// The warp-bake animation: the score morphs from ideal to performed spacing
// (each step is a full horizontal relayout — cheap in linear view mode).
constexpr int warpAnimSteps = 18;
constexpr int warpAnimStepMs = 40;

// The post-take beat grid: one line per quarter note (the sequencer's tick
// resolution is 480 per quarter).
constexpr int beatGridTicks = 480;
} // namespace

OrchestrionNotationPaintView::OrchestrionNotationPaintView(QQuickItem *parent)
    : mu::notation::NotationPaintView(parent), m_fader([this] { update(); }),
      m_timingOverlay([this] { update(); }), m_follower(*this),
      m_kineticScroller([this](qreal physicalDx)
                        { return moveCanvasBy(physicalDx); })
{
  m_clock.start();
  m_autoPlayTimer.setInterval(16); // ~60 Hz
  m_autoPlayTimer.callOnTimeout(
      [this]
      {
        const double nowMs = static_cast<double>(m_clock.elapsed());
        // The estimate must keep advancing (and coasting) even when the page
        // is still and no frames are being rendered.
        m_estimator.heartbeat(nowMs);
        fireDueAutoEvents(nowMs);
      });
  m_warpTimer.setInterval(warpAnimStepMs);
  m_warpTimer.callOnTimeout([this] { applyWarpStep(); });
}

bool OrchestrionNotationPaintView::tempoVisualizationEnabled() const
{
  return sequencerConfiguration()->tempoVisualizationEnabled();
}

void OrchestrionNotationPaintView::subscribe(
    const IOrchestrionSequencer &sequencer,
    const IModifiableItemRegistry &registry)
{
  sequencer.ChordTransitions().onReceive(
      this,
      [this](std::map<TrackIndex, ChordTransition> transitions)
      {
        OnTransitions(transitions);
        update();
      });

  registry.ModifiedChanged().onNotify(this, [this] { update(); });

  // The raw controller velocity of each gesture, for the dynamics scoring:
  // the event precedes the transitions batch the gesture causes.
  sequencer.HandNoteEvents().onReceive(
      this,
      [this](const AutoPlayEvent &event)
      {
        if (event.type == NoteEventType::noteOn)
          m_pendingHandVelocity[event.isLeftHand ? 1 : 0] = event.velocity;

        // Record the take's raw events, for the post-take replay — part of
        // the grading apparatus (without it, the play button is
        // plain metronomic playback).
        if (!sequencerConfiguration()->gradingEnabled())
          return;
        // A note-on while the previous take's stats are stale begins a new
        // take (this event precedes the transitions batch that restarts the
        // stats).
        if (orchestrion()->player()->IsReplaying())
          return; // the replay's own events aren't a new performance
        const bool newTake = event.type == NoteEventType::noteOn &&
                             (m_timingStatsStale || m_replayEvents.empty());
        if (newTake)
        {
          m_replayEvents.clear();
          m_replayClock.restart();
          m_replayStartTick = std::numeric_limits<int>::max();
          m_takeOver = false;
          // From here on, the play button is the metronomic playback again —
          // until this take, in turn, is over.
          orchestrion()->player()->SetReplayTake(std::nullopt);
        }
        else if (m_replayEvents.empty())
          return; // a stray release before any take began
        m_replayEvents.push_back(
            {newTake ? 0 : static_cast<int>(m_replayClock.elapsed()),
             event.type, event.isLeftHand, event.velocity});
        // A release arriving after the take ended still belongs to it: keep
        // the pushed copy complete, so replayed notes don't ring forever.
        if (m_timingStatsStale && !newTake)
          pushReplayTake();
      });

  if (const auto &transitions = sequencer.GetCurrentTransitions();
      !transitions.empty())
    OnTransitions(transitions);

  sequencer.AboutToJumpPosition().onReceive(this,
                                            [this](auto)
                                            {
                                              m_kineticScroller.stop();
                                              // The position jumps: forget the
                                              // tempo estimate and re-frame at
                                              // the new location on the next
                                              // transitions.
                                              // The jump's transitions batch
                                              // repopulates the ledger.
                                              m_autoTrackTargets.clear();
                                              m_readingFocus.clear();
                                              m_follower.jump();
                                              m_focusUtick.reset();
                                              m_resumeUtick.reset();
                                              m_estimator.reset();
                                              m_loudness.clear();
                                              m_tempoVizModel.clear();
                                              m_boxes.clear();
                                              m_fader.clear();
                                              // An interruption: the timing
                                              // stats restart when playing
                                              // resumes (readable until then).
                                              m_timingStatsStale = true;
                                              endTake();
                                              update();
                                            });
}

void OrchestrionNotationPaintView::OnTransitions(
    const std::map<TrackIndex, ChordTransition> &transitions)
{
  // Onsets driving the estimate, grouped per hand (= staff): the voices on a
  // staff are played by the same gestures, so they share one tracker. Each
  // hand's sounding onset is a tempo observation for it, at its
  // playback-unrolled tick. (The page scroll is fed per voice, through
  // m_readingFocus, below.)
  std::map<int /*staff*/, double /*utick*/> soundingTicks;
  // The gesture's controller velocity per hand, when the device measures one:
  // the dynamics judgments' input.
  std::map<int /*staff*/, double> soundingVelocity;
  std::map<int /*staff*/, int> presentScoreTicks; // engraved position
  std::map<int /*staff*/, const mu::engraving::EngravingItem *> presentAnchors;
  // Where to place a hand's timing gauge if its onset gets judged: the union
  // of the boxes of the notes it struck this batch, plus the staff's edges so
  // the gauge keeps clear of the staff lines.
  struct StaffHit
  {
    QRectF rect;
    double spatium = 1.0;
    double staffTop = 0.0;
    double staffBottom = 0.0;
    // The struck notes' engraving items, for the tempo-warped shadow copies.
    std::vector<mu::engraving::EngravingItem *> items;
  };
  std::map<int /*staff*/, StaffHit> staffHits;

  for (const auto &[track, transition] : transitions)
  {
    if (GetPastChord(transition))
    {
      // The note on this track just ended. Instead of dropping its highlight
      // instantly, hand it to the fader so it fades out (while any new note
      // below lights up at full strength).
      if (const auto it = m_boxes.find(track.value); it != m_boxes.end())
      {
        m_fader.add(it->second);
        m_boxes.erase(it);
      }
    }

    const IMelodySegment *present = GetPresentThing(transition);
    const IChord *future = GetFutureChord(transition);

    // The onset of a thing on this track — its engraved x (the follow's
    // coordinate) and unrolled tick — taken from the segment element rather
    // than the hugging box, whose width includes ties. Nothing for a voice
    // blank, which no engraving segment represents.
    const auto onsetOf =
        [&](const IMelodySegment *item) -> std::optional<ReadingFocus::Onset>
    {
      if (!item)
        return std::nullopt;
      const auto *segment = chordRegistry()->GetSegment(item);
      const auto *el = segment ? segment->element(track.value) : nullptr;
      if (!el)
        return std::nullopt;
      return ReadingFocus::Onset{item->GetBeginTick().withRepeats,
                                 el->pageBoundingRect().center().x()};
    };
    // Fed for every track in the batch, gap rests included: the sequencer has
    // moved the hand there, visible or not.
    m_readingFocus.onTransition(track.staffIndex(), track.value,
                                onsetOf(present), onsetOf(future));

    const auto thing = present ? present : future;
    if (!thing)
      continue;

    const auto segment = chordRegistry()->GetSegment(thing);
    if (!segment)
      // Could be a voice blank, which isn't represented by an engraving segment
      continue;
    const std::vector<mu::engraving::EngravingItem *> items =
        getRelevantItems(track, segment);
    const auto invisible = std::all_of(
        items.begin(), items.end(),
        [](const auto item)
        {
          if (const auto rest = dynamic_cast<mu::engraving::Rest *>(item))
            return rest->isGap();
          else
            return false;
        });
    if (invisible)
      continue;
    mu::engraving::RectF huggingBox;
    for (const auto item : items)
      huggingBox = huggingBox.united(item->pageBoundingRect());
    const auto huggingRect = huggingBox.toQRectF();
    const auto spatium = items.front()->spatium();
    const bool active = present != nullptr;
    Highlight &box = m_boxes[track.value];
    box.rect = huggingRect.adjusted(-spatium, -spatium, spatium, spatium);
    box.spatium = spatium;
    // Mahogany theme color; a ringing note is highlighted at full strength,
    // the next note sits faintly pre-lit. (Decaying the intensity over the
    // note's ring would need a render timer feeding a ring level here.)
    constexpr auto mahogany = "#5A2B25";

    box.color = QColor(mahogany);
    box.intensity = active ? 1.0 : 0.3;

    if (active)
    {
      StaffHit &hit = staffHits[track.staffIndex()];
      hit.rect = hit.rect.isNull() ? box.rect : hit.rect.united(box.rect);
      hit.spatium = spatium;
      hit.items.insert(hit.items.end(), items.begin(), items.end());
      if (const mu::engraving::System *system = segment->measure()->system())
      {
        hit.staffTop = system->staffYpage(track.staffIndex());
        hit.staffBottom = hit.staffTop + 4.0 * spatium; // the 5 staff lines
      }
      else
      {
        hit.staffTop = hit.rect.top();
        hit.staffBottom = hit.rect.bottom();
      }
    }

    // This track's struck chord, for the estimate. A rest is "present" too —
    // the voice moves onto it as the chord before it is released — but that
    // is not an onset: fed as one, it lands a few ms after the release, and
    // the tracker reads a note's worth of ticks over next to no time as a
    // tempo of thousands of bpm.
    if (const auto el = segment->element(track.value);
        el && GetPresentChord(transition))
    {
      // Collapse a staff's voices into one onset (its latest), carrying the
      // playback-unrolled tick — continuous through repeats, voltas and jumps
      // — for the musical-tempo readout and the timing judgments.
      const int hand = track.staffIndex();
      const double utick =
          static_cast<double>(present->GetBeginTick().withRepeats);
      // The gesture's controller velocity (cached from HandNoteEvents just
      // before this batch), for the dynamics judgments.
      const std::optional<float> &velocity =
          m_pendingHandVelocity[hand > 0 ? 1 : 0];
      const auto it = soundingTicks.find(hand);
      if (it == soundingTicks.end() || utick > it->second)
      {
        soundingTicks[hand] = utick;
        if (velocity)
          soundingVelocity[hand] = *velocity;
        else
          soundingVelocity.erase(hand);
        presentScoreTicks[hand] = segment->tick().ticks();
        presentAnchors[hand] = el;
      }
    }
  }

  // A replay of the finished take: the review visuals (marks, ribbon, warp,
  // stats) stay frozen for comparison with what is heard; only the follower
  // (scroll) and the note highlights track the replayed events.
  const bool replaying = orchestrion()->player()->IsReplaying();

  // Which hands are about to have their estimate restarted (they had coasted
  // to a stop): their loudness curve restarts with it. Asked before the
  // estimator consumes this batch, which is what clears the coast.
  std::vector<int> resumingHands;
  for (const auto &[staff, utick] : soundingTicks)
    if (m_estimator.isCoasting(staff))
      resumingHands.push_back(staff);

  const double nowMs = static_cast<double>(m_clock.elapsed());
  const PositionEstimator::Feedback feedback =
      m_estimator.onOnsets(nowMs, soundingTicks);
  const auto dynamicsJudgments =
      judgeDynamics(nowMs, soundingVelocity, resumingHands);
  // The machine-played hand is tracked (its onsets feed the estimate and the
  // viz) but never judged, and it is synchronised with the performer by
  // construction: it is not a performance.
  const int autoStaff = autoPlayedStaff();
  // Orchestrion is played with two hands: staff 0 is the right, staff 1 the
  // left. The estimator declines unless both are warmed up and playing.
  const auto handSync = autoStaff < 0
                            ? m_estimator.asynchrony(0, 1, nowMs)
                            : std::optional<PositionEstimator::Judgment>{};

  // What the page keeps in view — the leading hand's reading (see
  // ReadingFocus) — and the barrier ahead of it.
  const std::optional<ReadingFocus::Onset> focus = m_readingFocus.focus();
  const std::optional<ReadingFocus::Onset> leader =
      m_readingFocus.leaderOnset();
  const std::optional<BarrierAhead> ahead =
      focus ? nextBarrier(focus->utick) : std::nullopt;
  m_focusUtick = focus ? std::optional{focus->utick} : std::nullopt;
  m_resumeUtick = ahead ? ahead->resumeUtick : std::nullopt;
  const auto xOf = [](const std::optional<ReadingFocus::Onset> &onset)
  { return onset ? std::optional{onset->x} : std::nullopt; };
  m_follower.onEvents(!soundingTicks.empty(), xOf(leader ? leader : focus),
                      xOf(focus),
                      ahead ? std::optional{ahead->barrier} : std::nullopt);

  if (replaying)
  {
    // The recorded take already contains the auto hand's events: keep the
    // trigger quiet so they don't fire twice.
    m_autoOffTick.reset();
    m_autoOnTick.reset();
  }
  else
    updateAutoTargets(transitions);

  // Feed the debug tempo strip: the onsets the estimate reacted to, and each
  // hand's re-fitted curve (replaced wholesale per onset).
  for (const auto &[staff, tMs] : feedback.onsetTMs)
  {
    m_tempoVizModel.addOnset(tMs, staff);
    m_tempoVizModel.setSmoothedCurve(staff,
                                     m_estimator.smoothedBpmCurve(staff));
  }
  // The cached velocities were for this batch only.
  m_pendingHandVelocity[0].reset();
  m_pendingHandVelocity[1].reset();

  // Track the take's earliest struck score tick: where its replay rewinds to.
  if (!replaying)
    for (const auto &[staff, tick] : presentScoreTicks)
      m_replayStartTick = std::min(m_replayStartTick, tick);

  // Playing has resumed after an interruption: the error stats start a fresh
  // take. (It stays readable while interrupted; only the resume clears it.)
  if (m_timingStatsStale && !soundingTicks.empty() && !replaying)
  {
    m_timingOverlay.reset();
    m_timingStatsStale = false;
    m_finalScoreShown = false;
    m_takeOnsetRecords.clear();
    clearPerformanceWarp();
    dismissFinalScore();
    emit smoothingTunerVisibleChanged();
  }

  // The game feedback: an error gauge next to the struck notes — above the
  // staff for the right hand, below for the left — and the hand's revised
  // judgments into the overlay (moving still-showing markers, re-binning the
  // box plot). The newest onset's judgment is the window's last.
  if (sequencerConfiguration()->gradingEnabled() && !replaying)
  {
    // Every sounding manual onset gets its marks (gauge, ribbon point, take
    // record) up front, in a pending state: the judgments fill them in as
    // they arrive — retroactively for the take's first onsets, which are
    // only judged once the spline has warmed up.
    for (const auto &[staff, tMs] : feedback.onsetTMs)
    {
      if (staff == autoStaff)
        continue;
      if (const auto it = staffHits.find(staff); it != staffHits.end())
      {
        const StaffHit &hit = it->second;
        const bool below = staff > 0; // left hand
        m_timingOverlay.addGauge(staff, tMs, hit.rect, hit.spatium, below,
                                 below ? hit.staffBottom : hit.staffTop,
                                 hit.items);
      }
      if (const auto tickIt = presentScoreTicks.find(staff);
          tickIt != presentScoreTicks.end())
        m_takeOnsetRecords.push_back(
            {staff, tMs, tickIt->second, soundingTicks.at(staff),
             static_cast<double>(m_replayClock.elapsed()),
             presentAnchors.at(staff)});
    }
    for (const auto &[staff, window] : feedback.judgments)
      if (!window.empty() && staff != autoStaff)
        m_timingOverlay.updateJudgments(staff, window);
    if (handSync && sequencerConfiguration()->handSyncScoreEnabled())
      m_timingOverlay.addSyncSample(handSync->tMs, handSync->errorMs);
    if (sequencerConfiguration()->dynamicsScoreEnabled())
      for (const auto &[staff, window] : dynamicsJudgments)
        m_timingOverlay.updateDynamicsJudgments(staff, window);
  }

  // End of the piece: nothing is sounding (notes *and* rests) and nothing is
  // upcoming on any voice — the last notes were just released — so raise the
  // final-score banner, once. (While a chord sounds or a rest passes, its
  // transition is a *present* state carrying no future, so both must be
  // absent to distinguish the true end; a mid-piece release holds a future
  // chord or a present rest instead.)
  if (!m_finalScoreShown && !replaying &&
      sequencerConfiguration()->gradingEnabled())
    if (const auto sequencer = orchestrion()->sequencer())
    {
      const auto &current = sequencer->GetCurrentTransitions();
      const bool done = !current.empty() &&
                        std::all_of(current.begin(), current.end(),
                                    [](const auto &entry) {
                                      return !GetFutureChord(entry.second) &&
                                             !GetPresentThing(entry.second);
                                    });
      if (done)
      {
        // The take is over: review time. Ends (and re-fits) the take before
        // the banner reads its score, so the verdict is the full-hindsight
        // one.
        endTake();
        if (const auto score = m_timingOverlay.takeFinalScore())
        {
          m_finalScoreShown = true;
          m_finalScore = *score;
          m_finalScoreBreakdown = m_timingOverlay.takeScoreBreakdown();
          m_finalScoreMetrics.clear();
          for (const auto &metric : m_timingOverlay.takeScoreMetrics())
            m_finalScoreMetrics.append(QVariantMap{{"label", metric.label},
                                                   {"score", metric.score},
                                                   {"detail", metric.detail}});
          emit finalScoreChanged();
          // endTake's visibility emit preceded m_finalScoreShown: re-emit.
          emit smoothingTunerVisibleChanged();
        }
      }
    }
}

std::map<int, std::vector<PositionEstimator::Judgment>>
OrchestrionNotationPaintView::judgeDynamics(
    double nowMs, const std::map<int, double> &velocities,
    const std::vector<int> &resumingHands)
{
  // The same retrospective principle as the timing judgments, over a loudness
  // curve instead of a position one: a gesture's velocity is (re-)measured
  // against the smoothed swell as later gestures refine it. The residual is
  // already the error (a velocity fraction) — no time conversion. Only
  // gestures whose device measures velocity contribute.
  std::map<int, std::vector<PositionEstimator::Judgment>> windows;
  for (const auto &[staff, velocity] : velocities)
  {
    auto it = m_loudness.find(staff);
    if (it == m_loudness.end())
      it = m_loudness
               .emplace(staff,
                        PositionSmoother{
                            sequencerConfiguration()->tempoSmoothingMemory()})
               .first;
    else if (std::find(resumingHands.begin(), resumingHands.end(), staff) !=
             resumingHands.end())
      // The performer stopped and resumed: the wound-down swell says nothing
      // about the dynamics they resume with.
      it->second.reset();
    PositionSmoother &smoother = it->second;
    smoother.addObservation(nowMs, velocity);
    if (smoother.knots().size() < dynamicsMinKnots)
      continue;
    std::vector<PositionEstimator::Judgment> window;
    const auto residuals = smoother.residuals();
    window.reserve(residuals.size());
    for (const auto &residual : residuals)
      window.push_back({residual.time, residual.error});
    windows[staff] = std::move(window);
  }
  return windows;
}

void OrchestrionNotationPaintView::updateAutoTargets(
    const std::map<TrackIndex, ChordTransition> &batch)
{
  const int autoStaff = autoPlayedStaff();
  if (autoStaff < 0)
    return;

  // Refresh the ledger entries this batch brings for the auto staff.
  for (const auto &[track, transition] : batch)
  {
    if (track.staffIndex() != autoStaff)
      continue;
    AutoTargets &targets = m_autoTrackTargets[track.value];
    const IChord *present = GetPresentChord(transition);
    targets.offTick =
        present ? std::make_optional<double>(present->GetEndTick().withRepeats)
                : std::nullopt;
    const IChord *future = GetFutureChord(transition);
    targets.onTick =
        future ? std::make_optional<double>(future->GetBeginTick().withRepeats)
               : std::nullopt;
  }

  // Aggregate: the auto hand's earliest release and strike across its voices,
  // in playback-unrolled ticks — the coordinate the manual hands' estimate
  // lives in.
  std::optional<double> offTick;
  std::optional<double> onTick;
  for (const auto &[track, targets] : m_autoTrackTargets)
  {
    if (targets.offTick)
      offTick =
          offTick ? std::min(*offTick, *targets.offTick) : targets.offTick;
    if (targets.onTick)
      onTick = onTick ? std::min(*onTick, *targets.onTick) : targets.onTick;
  }
  m_autoOffTick = offTick;
  m_autoOnTick = onTick;
  // Poll only while there is something to fire.
  if (m_autoOffTick || m_autoOnTick)
  {
    if (!m_autoPlayTimer.isActive())
      m_autoPlayTimer.start();
  }
  else
    m_autoPlayTimer.stop();
}

int OrchestrionNotationPaintView::autoPlayedStaff() const
{
  // While auto-play is not exposed it stays inactive, whatever staff the
  // setting holds (see IOrchestrionSequencerConfiguration::autoPlayExposed).
  return sequencerConfiguration()->autoPlayExposed()
             ? sequencerConfiguration()->autoPlayedStaff()
             : -1;
}

void OrchestrionNotationPaintView::fireDueAutoEvents(double nowMs)
{
  // Fire the auto hand's due events once the manual hands' estimated position
  // reaches them. Each target fires once; the resulting transitions bring the
  // next ones. When the performer stops, the estimate coasts to a halt and
  // the auto hand halts with it.
  const int autoStaff = autoPlayedStaff();
  if (autoStaff < 0 || (!m_autoOnTick && !m_autoOffTick))
  {
    m_autoPlayTimer.stop();
    return;
  }

  std::optional<double> manualTicks;
  for (int staff = 0; staff < 2; ++staff)
  {
    if (staff == autoStaff)
      continue;
    if (const auto tick = m_estimator.tickAt(staff, nowMs))
      manualTicks = manualTicks ? std::max(*manualTicks, *tick) : *tick;
  }
  if (!manualTicks)
    return;

  const auto fire = [this, autoStaff](bool noteOn)
  {
    // Deferred out of the frame tick: the events it causes re-enter this
    // view with fresh targets.
    QTimer::singleShot(
        0, this,
        [this, noteOn]
        {
          const int staff = autoPlayedStaff();
          const auto sequencer = orchestrion()->sequencer();
          if (staff < 0 || !sequencer)
            return;
          sequencer->OnInputEvent(
              noteOn ? NoteEventType::noteOn : NoteEventType::noteOff,
              staff > 0 ? leftHandPitch : rightHandPitch, std::nullopt);
        });
  };

  if (m_autoOnTick && *manualTicks >= *m_autoOnTick)
  {
    // Advancing releases the previous chord itself: the pending noteOff is
    // superseded.
    m_autoOnTick.reset();
    m_autoOffTick.reset();
    fire(true);
  }
  else if (m_autoOffTick && *manualTicks >= *m_autoOffTick)
  {
    m_autoOffTick.reset();
    fire(false);
  }
}

void OrchestrionNotationPaintView::endTake()
{
  if (!m_takeOver)
  {
    m_takeOver = true;
    // The live judgments freeze with whatever hindsight the bounded
    // smoothing window happened to give them; the take's final verdicts
    // (ribbon, marks, stats, warp) come from one full-hindsight re-fit —
    // the same fit the γ slider explores, so the slider then only changes
    // anything when γ actually changes.
    refitTakeJudgments();
  }
  pushReplayTake();
  bakePerformanceWarp();
}

void OrchestrionNotationPaintView::refitTakeJudgments()
{
  if (m_takeOnsetRecords.empty())
    return;
  const double memory = sequencerConfiguration()->tempoSmoothingMemory();

  // Per staff, the take's raw observations, in onset order.
  std::map<int, std::vector<std::pair<double, double>>> observations;
  for (const TakeOnsetRecord &record : m_takeOnsetRecords)
    observations[record.staff].emplace_back(record.tMs, record.utick);
  for (const auto &[staff, obs] : observations)
  {
    const auto window = PositionEstimator::refitTake(obs, memory);
    if (!window.empty())
      m_timingOverlay.updateJudgments(staff, window);
  }
}

void OrchestrionNotationPaintView::pushReplayTake()
{
  const auto player = orchestrion()->player();
  if (!sequencerConfiguration()->gradingEnabled())
  {
    player->SetReplayTake(std::nullopt);
    return;
  }
  if (m_replayEvents.empty() ||
      m_replayStartTick == std::numeric_limits<int>::max())
    return;
  switch (orchestrion()->playMode())
  {
  case PlayMode::replayPerformance:
    player->SetReplayTake(ReplayTake{m_replayStartTick, m_replayEvents});
    break;
  case PlayMode::replayFittedTempo:
    player->SetReplayTake(ReplayTake{m_replayStartTick, fittedTempoEvents()});
    break;
  case PlayMode::metronome:
    player->SetReplayTake(std::nullopt);
    break;
  }
}

std::vector<ReplayEvent> OrchestrionNotationPaintView::fittedTempoEvents() const
{
  // Per hand, the take onsets' final fitted errors over the recording's
  // clock. (Staff 0 is the right hand, the rest the left — the same grouping
  // that routes the input events.)
  std::map<bool /*isLeft*/, std::vector<std::pair<double, double>>> errors;
  for (const TakeOnsetRecord &record : m_takeOnsetRecords)
    if (const auto error =
            m_timingOverlay.takeErrorAt(record.staff, record.tMs))
      errors[record.staff > 0].emplace_back(record.eventMs, *error);
  for (auto &[isLeft, series] : errors)
    std::sort(series.begin(), series.end());

  // The fitted error at any event time, by linear interpolation between the
  // hand's onsets (clamped at the take's ends). The auto-played hand has no
  // judgments, hence no series: its events replay as recorded.
  const auto errorAt = [&errors](bool isLeft, double ms)
  {
    const auto it = errors.find(isLeft);
    if (it == errors.end() || it->second.empty())
      return 0.0;
    const auto &series = it->second;
    if (ms <= series.front().first)
      return series.front().second;
    if (ms >= series.back().first)
      return series.back().second;
    const auto next =
        std::lower_bound(series.begin(), series.end(), std::make_pair(ms, 0.0));
    const auto prev = std::prev(next);
    const double span = next->first - prev->first;
    const double frac = span > 0.0 ? (ms - prev->first) / span : 0.0;
    return prev->second + frac * (next->second - prev->second);
  };

  std::vector<ReplayEvent> events = m_replayEvents;
  // error = actual − fitted, so the fitted arrival is the shift-back.
  double lastMs[2] = {-std::numeric_limits<double>::infinity(),
                      -std::numeric_limits<double>::infinity()};
  for (ReplayEvent &event : events)
  {
    double ms = event.ms - errorAt(event.isLeftHand, event.ms);
    // Keep each hand's event order: a release hopping over the next strike
    // would make the sequencer cut the wrong chord.
    double &prev = lastMs[event.isLeftHand ? 1 : 0];
    ms = std::max(ms, prev);
    prev = ms;
    event.ms = static_cast<int>(std::lround(ms));
  }
  std::stable_sort(events.begin(), events.end(),
                   [](const ReplayEvent &a, const ReplayEvent &b)
                   { return a.ms < b.ms; });
  if (!events.empty())
  {
    const int firstMs = events.front().ms;
    for (ReplayEvent &event : events)
      event.ms -= firstMs;
  }
  return events;
}

void OrchestrionNotationPaintView::bakePerformanceWarp(bool animate)
{
  // The take is (or may be) over: the tuning slider shows/hides with it.
  emit smoothingTunerVisibleChanged();

  if (m_warpBaked ||
      !sequencerConfiguration()->timeProportionalSpacingEnabled() ||
      m_takeOnsetRecords.size() < 2)
    return;

  // Each onset's final, spline-settled fitted arrival time.
  struct Point
  {
    int scoreTick;
    double utick;
    double fittedMs;
    double tMs;
  };
  std::vector<Point> points;
  points.reserve(m_takeOnsetRecords.size());
  for (const TakeOnsetRecord &record : m_takeOnsetRecords)
    if (const auto error =
            m_timingOverlay.takeErrorAt(record.staff, record.tMs))
      points.push_back(
          {record.scoreTick, record.utick, record.tMs - *error, record.tMs});
  if (points.size() < 2)
    return;
  std::sort(points.begin(), points.end(),
            [](const Point &a, const Point &b) { return a.tMs < b.tMs; });

  // The take's constant-tempo reference: the line through its end onsets.
  const Point &first = points.front();
  const Point &last = points.back();
  const double span = last.fittedMs - first.fittedMs;
  if (span <= 0.0 || last.utick <= first.utick)
    return;
  const double refTempo = (last.utick - first.utick) / span;

  // The warped position per onset, folded back into score-tick space (a
  // repeat pass's tick offset cancels within the pass); the engraved bar of
  // a repeated section gets its *last* pass.
  std::map<int, std::pair<double /*tMs*/, double /*warped*/>> byScoreTick;
  for (const Point &p : points)
  {
    const double warpedUtick =
        first.utick + refTempo * (p.fittedMs - first.fittedMs);
    const double warped = warpedUtick - (p.utick - p.scoreTick);
    const auto it = byScoreTick.find(p.scoreTick);
    if (it == byScoreTick.end() || it->second.first < p.tMs)
      byScoreTick[p.scoreTick] = {p.tMs, warped};
  }

  // Assemble the layout table: sorted, monotonic, anchored so the first
  // onset keeps its place and the rest warps around it.
  std::vector<std::pair<int, double>> table;
  table.reserve(byScoreTick.size());
  double running = std::numeric_limits<double>::lowest();
  for (const auto &[tick, entry] : byScoreTick)
  {
    running = std::max(running, entry.second);
    table.emplace_back(tick, running);
  }
  const double shift = table.front().first - table.front().second;
  for (auto &[tick, warped] : table)
    warped += shift;

  m_warpTable = std::move(table);
  m_warpBaked = true;
  if (animate)
  {
    m_warpProgress = 0.0;
    m_warpTimer.start();
    return;
  }

  // Instant (re-tuning): apply the final table in one step.
  m_warpTimer.stop();
  m_warpProgress = 1.0;
  const auto masterNotation = globalContext()->currentMasterNotation();
  if (const auto master =
          masterNotation ? masterNotation->masterScore() : nullptr)
  {
    master->setLayoutTickWarp(m_warpTable);
    master->doLayout();
  }
  m_timingOverlay.setWarpProgress(1.0);
  constrainScorePosition();
  update();
}

void OrchestrionNotationPaintView::retuneTake()
{
  refitTakeJudgments();

  // The layout warp and the fitted-tempo replay derive from the fitted
  // errors: rebuild them in place, without the animation.
  if (m_warpBaked)
  {
    m_warpBaked = false;
    bakePerformanceWarp(false);
  }
  if (m_takeOver)
    pushReplayTake();
  update();
}

bool OrchestrionNotationPaintView::smoothingTunerVisible() const
{
  return (m_timingStatsStale || m_finalScoreShown) &&
         !m_takeOnsetRecords.empty() &&
         sequencerConfiguration()->gradingEnabled();
}

double OrchestrionNotationPaintView::tempoSmoothing() const
{
  return sequencerConfiguration()->tempoSmoothingMemory();
}

void OrchestrionNotationPaintView::setTempoSmoothing(double memory)
{
  memory = std::clamp(memory, 0.05, 0.98);
  if (qFuzzyCompare(memory, sequencerConfiguration()->tempoSmoothingMemory()))
    return;
  sequencerConfiguration()->setTempoSmoothingMemory(memory);
  m_estimator.setMemory(memory); // future takes fit live with it
  retuneTake();                  // this take re-fits right now
  emit tempoSmoothingChanged();
}

void OrchestrionNotationPaintView::applyWarpStep()
{
  m_warpProgress =
      std::min(1.0, m_warpProgress + 1.0 / static_cast<double>(warpAnimSteps));
  // Ease out: fast start, gentle landing.
  const double eased = 1.0 - std::pow(1.0 - m_warpProgress, 3.0);
  auto table = m_warpTable;
  for (auto &[tick, warped] : table)
    warped = (1.0 - eased) * tick + eased * warped;

  const auto masterNotation = globalContext()->currentMasterNotation();
  const auto master = masterNotation ? masterNotation->masterScore() : nullptr;
  if (!master)
  {
    m_warpTimer.stop();
    return;
  }
  master->setLayoutTickWarp(std::move(table));
  master->doLayout();
  m_timingOverlay.setWarpProgress(eased);
  if (m_warpProgress >= 1.0)
  {
    m_warpTimer.stop();
    constrainScorePosition();
  }
  update();
}

void OrchestrionNotationPaintView::clearPerformanceWarp()
{
  m_warpTimer.stop();
  m_warpProgress = 0.0;
  m_warpBaked = false;
  m_warpTable.clear();
  m_timingOverlay.setWarpProgress(0.0);
  const auto masterNotation = globalContext()->currentMasterNotation();
  const auto master = masterNotation ? masterNotation->masterScore() : nullptr;
  if (master && master->hasLayoutTickWarp())
  {
    master->setLayoutTickWarp({});
    master->doLayout();
    constrainScorePosition();
    update();
  }
}

void OrchestrionNotationPaintView::dismissFinalScore()
{
  if (m_finalScore < 0)
    return;
  m_finalScore = -1;
  m_finalScoreBreakdown.clear();
  m_finalScoreMetrics.clear();
  emit finalScoreChanged();
}

void OrchestrionNotationPaintView::connectFrameTick(QQuickWindow *window)
{
  if (m_frameTickConnection)
    disconnect(m_frameTickConnection);
  if (!window)
    return;
  // afterAnimating is emitted on the GUI thread once per frame, after the
  // declarative animations have advanced and before the scene graph is
  // synchronised — so the canvas placement and the repaint it dirties land in
  // that same frame, at the display's own cadence.
  m_frameTickConnection = connect(window, &QQuickWindow::afterAnimating, this,
                                  [this] { onFrameTick(); });
}

void OrchestrionNotationPaintView::onFrameTick()
{
  const double nowMs = static_cast<double>(m_clock.elapsed());
  // The estimate is told that time has passed, so a hand whose next note is
  // overdue winds down instead of extrapolating for ever; then the scroll
  // advances in step with the display (asking the estimate about the jump
  // ahead, if any).
  m_estimator.heartbeat(nowMs);
  m_follower.frameTick();

  // The debug tempo strip's live trace: each tracked hand's current tempo.
  if (!sequencerConfiguration()->tempoVisualizationEnabled())
    return;
  std::vector<TempoVizModel::HandTempo> samples;
  for (int staff = 0; staff < 2; ++staff)
    if (const auto bpm = m_estimator.bpm(staff))
      samples.push_back({staff, *bpm, m_estimator.isCoasting(staff)});
  if (!samples.empty())
    m_tempoVizModel.addTempoSample(nowMs, samples);
}

double OrchestrionNotationPaintView::anchorX() const
{
  // The inverse of centerOn()'s placement (short of its clamping).
  return viewport().left() +
         ScoreFollower::anchorFrac * width() / currentScaling();
}

void OrchestrionNotationPaintView::centerOn(double logicalX)
{
  // constrainScorePosition() (via onMatrixChanged) would otherwise pull the
  // viewport back to hug the content; yield to us while we place the canvas.
  m_drivingScroll = true;

  const double scaling = currentScaling();
  const double logicalWidth = width() / scaling;
  // Rest the anchor at the follower's playhead fraction, but never past the
  // max-padding limit (so near the start/end of the score the anchor drifts
  // off its spot rather than opening a gap wider than a manual zoom-out would
  // allow).
  const double leftX =
      clampLeftX(logicalX - ScoreFollower::anchorFrac * logicalWidth, scaling);

  const auto content = notationContentRect();
  const double emptyAbovePhysical =
      (height() - content.height() * scaling) / 2.;
  const double topY = content.top() - emptyAbovePhysical / scaling;

  const bool moved = moveCanvasToPosition(muse::PointF{leftX, topY});

  m_drivingScroll = false;
  // The follow runs every frame, but the page stands still between turns:
  // repainting then would re-render the whole score (this is a
  // QQuickPaintedItem) for an identical picture, 60 times a second.
  if (moved)
    update();
}

std::vector<mu::engraving::EngravingItem *>
OrchestrionNotationPaintView::getRelevantItems(
    TrackIndex track, const mu::engraving::Segment *segment) const
{
  const auto chord =
      dynamic_cast<const mu::engraving::Chord *>(segment->element(track.value));
  using NoteVector = std::vector<mu::engraving::Note *>;
  const NoteVector notes = chord ? chord->notes() : NoteVector{};
  std::vector<mu::engraving::EngravingItem *> items;
  std::for_each(notes.begin(), notes.end(),
                [&](mu::engraving::Note *note)
                {
                  while (note)
                  {
                    items.emplace_back(note);
                    auto tie = note->tieFor();
                    if (tie)
                      items.emplace_back(tie);
                    note = tie ? tie->endNote() : nullptr;
                  }
                });

  if (notes.empty())
  {
    // Get all consecutive rests, ignoring elements other than chords such as
    // bars, clefs, etc.
    while (segment)
    {
      auto item = segment->element(track.value);
      if (dynamic_cast<mu::engraving::Chord *>(item))
        break;
      if (const auto rest = dynamic_cast<mu::engraving::Rest *>(item))
        items.emplace_back(rest);
      segment = segment->next();
    }
  }
  return items;
}

void OrchestrionNotationPaintView::onLoadNotation(
    mu::notation::INotationPtr notation)
{
  mu::notation::NotationPaintView::onLoadNotation(std::move(notation));
  // We want hover events, which NotationPaintView::onLoadNotation may have set
  // to false.
  setAcceptHoverEvents(true);
}

bool OrchestrionNotationPaintView::eventFilter(QObject *watched, QEvent *event)
{
  const auto type = event->type();
  if (type == QEvent::HoverMove || type == QEvent::MouseMove ||
      type == QEvent::MouseButtonPress)
  {
    bool inScope = false;
    for (QObject *o = watched; o; o = o->parent())
      if (o == this)
      {
        inScope = true;
        break;
      }

    if (inScope)
    {
      // Gate on real cursor movement: Qt synthesises hover events as the
      // scene graph updates (e.g. when a child runs an infinite opacity
      // animation), and those carry the unchanged cursor position.
      if (type == QEvent::MouseButtonPress)
        emit mouseActivity();
      else
      {
        const QPoint pos = QCursor::pos();
        if (pos != m_lastCursorPos)
        {
          m_lastCursorPos = pos;
          emit mouseActivity();
        }
      }
    }
  }

  if (watched == this)
  {
    const bool wasDraggingLoopFlag = m_draggedLoopBoundary.has_value();
    const auto mouseEvent = static_cast<QMouseEvent *>(event);
    switch (event->type())
    {
    case QEvent::MouseButtonPress:
      onMousePressed(mouseEvent->position(), mouseEvent->modifiers(),
                     mouseEvent->button());
      break;
    case QEvent::MouseMove:
      onMouseDragged(mouseEvent->position(), mouseEvent->buttons());
      break;
    case QEvent::MouseButtonRelease:
      onMouseReleased(mouseEvent->button());
      break;
    case QEvent::HoverMove:
      onMouseMoved(mouseEvent->position());
      break;
    default:
      break;
    }

    // A loop-flag drag owns the mouse: swallow its events so the base view
    // doesn't also pan the canvas or interact with the score.
    if ((m_draggedLoopBoundary.has_value() || wasDraggingLoopFlag) &&
        (type == QEvent::MouseButtonPress || type == QEvent::MouseMove ||
         type == QEvent::MouseButtonRelease))
      return true;
  }

  return mu::notation::NotationPaintView::eventFilter(watched, event);
}

void OrchestrionNotationPaintView::onMousePressed(
    const QPointF &pos, Qt::KeyboardModifiers modifiers, Qt::MouseButton button)
{
  m_kineticScroller.stop(); // a click on the score halts an in-progress glide
  m_follower.suspend();     // ...and hands auto-scroll control back to the user
  m_timingStatsStale = true; // timing stats restart when playing resumes
  endTake();                 // an interruption ends the take: review time
  const muse::PointF logicPos = toLogical(pos);
  const auto interaction = notationInteraction();
  const mu::notation::EngravingItem *hitElement =
      interaction ? interaction->hitElement(logicPos, hitWidth()) : nullptr;

  if (button == Qt::RightButton)
  {
    m_contextMenuTarget = loopBoundariesController()->chordTicks(hitElement);
    emit contextMenuTargetChanged();
    emit contextMenuRequested(pos);
    return;
  }

  if (button == Qt::LeftButton && modifiers.testFlag(Qt::ShiftModifier))
  {
    if (const auto ticks = loopBoundariesController()->chordTicks(hitElement))
      loopBoundariesController()->onChordShiftClicked(*ticks);
    return;
  }

  if (button == Qt::LeftButton && modifiers == Qt::NoModifier)
    if ((m_draggedLoopBoundary = loopFlagAt(logicPos)))
      return;

  interactionProcessor()->onMousePressed(logicPos, hitWidth());

  // A plain left-drag starting on empty background pans the canvas (the base
  // view does the panning); track it so the release can add a kinetic throw.
  // Pressing an element, or holding a modifier, is selection — not panning.
  m_canvasDragging =
      button == Qt::LeftButton && modifiers == Qt::NoModifier && !hitElement;
  if (m_canvasDragging)
  {
    m_lastDragPos = pos;
    m_kineticScroller.beginDrag();
  }
}

void OrchestrionNotationPaintView::onMouseDragged(const QPointF &pos,
                                                  Qt::MouseButtons buttons)
{
  if (m_draggedLoopBoundary && (buttons & Qt::LeftButton))
  {
    dragLoopBoundaryTo(toLogical(pos));
    return;
  }

  if (!m_canvasDragging || !(buttons & Qt::LeftButton))
    return;
  // The base view pans the canvas to follow the cursor; we only feed the
  // horizontal cursor delta to the scroller so it can throw on release.
  m_kineticScroller.addDragSample(pos.x() - m_lastDragPos.x());
  m_lastDragPos = pos;
}

void OrchestrionNotationPaintView::onMouseReleased(Qt::MouseButton button)
{
  if (button == Qt::LeftButton && m_draggedLoopBoundary)
  {
    m_draggedLoopBoundary.reset();
    return;
  }

  if (button != Qt::LeftButton || !m_canvasDragging)
    return;
  m_canvasDragging = false;
  m_kineticScroller.endDrag();
}

bool OrchestrionNotationPaintView::contextMenuHasTarget() const
{
  return m_contextMenuTarget.has_value();
}

void OrchestrionNotationPaintView::contextMenuSetLoopStart()
{
  if (m_contextMenuTarget)
    loopBoundariesController()->setLoopStart(m_contextMenuTarget->start);
}

void OrchestrionNotationPaintView::contextMenuSetLoopEnd()
{
  if (m_contextMenuTarget)
    loopBoundariesController()->setLoopEnd(m_contextMenuTarget->end);
}

void OrchestrionNotationPaintView::clearLoop()
{
  loopBoundariesController()->clearLoop();
}

void OrchestrionNotationPaintView::onMouseMoved(const QPointF &pos)
{
  const muse::PointF logicPos = toLogical(pos);

  const bool overLoopFlag = loopFlagAt(logicPos).has_value();
  if (overLoopFlag != m_loopFlagCursor)
  {
    if (overLoopFlag)
      QApplication::setOverrideCursor(Qt::SizeHorCursor);
    else
      QApplication::restoreOverrideCursor();
    m_loopFlagCursor = overLoopFlag;
  }
  if (overLoopFlag)
    // Keep the interaction processor from replacing the resize cursor with
    // the pointing hand of an element underneath the flag.
    return;

  interactionProcessor()->onMouseMoved(logicPos, hitWidth());

  // A timing gauge under the cursor tells its onset's error; anywhere else
  // along the deviation curve tells the smoothed tempo there; otherwise fall
  // back to the note-info debug tooltip (which has its own toggle).
  QString timingInfo;
  QPointF timingPos = pos;
  int placement = 0; // 0 = at the cursor
  if (sequencerConfiguration()->gradingEnabled())
  {
    const QPointF logical(logicPos.x(), logicPos.y());
    // The hovered onset also reveals its coloured shadow copy (gliding out
    // from the engraved notes to its error position); its tooltip anchors
    // beside the noteheads, on the copy-free side.
    m_timingOverlay.updateHover(logical);
    if (const auto tip = m_timingOverlay.gaugeTipAt(logical))
    {
      timingInfo = tip->text;
      const muse::PointF anchor =
          fromLogical(muse::PointF(tip->anchor.x(), tip->anchor.y()));
      timingPos = QPointF(anchor.x(), anchor.y());
      placement = tip->leftOfAnchor ? 1 : 2;
    }
    else
      timingInfo = m_timingOverlay.ribbonInfoAt(logical);
  }
  if (!timingInfo.isEmpty())
  {
    m_hoveredNoteInfoPlacement = placement;
    setHoveredNoteInfo(timingInfo, timingPos);
  }
  else
  {
    m_hoveredNoteInfoPlacement = 0;
    if (sequencerConfiguration()->noteInfoTooltipEnabled())
      updateHoveredNoteInfo(pos);
    else if (!m_hoveredNoteInfo.isEmpty())
      setHoveredNoteInfo({}, pos);
  }
}

std::optional<mu::notation::LoopBoundaryType>
OrchestrionNotationPaintView::loopFlagAt(const muse::PointF &logicPos) const
{
  const auto masterNotation = globalContext()->currentMasterNotation();
  if (!masterNotation || !masterNotation->playback()->loopBoundaries().enabled)
    return std::nullopt;

  // Inflate horizontally by half the marker width so the thin flag line is
  // comfortable to grab.
  const auto contains = [&logicPos](const muse::RectF &rect)
  {
    if (rect.isEmpty())
      return false;
    const auto pad = rect.width() / 2;
    return muse::RectF{rect.left() - pad, rect.top(), rect.width() + 2 * pad,
                       rect.height()}
        .contains(logicPos);
  };

  const auto inRect = loopInMarkerRect();
  const auto outRect = loopOutMarkerRect();
  const bool inHit = contains(inRect);
  const bool outHit = contains(outRect);
  if (inHit && outHit)
    // Overlapping flags (a very short loop): pick the nearer one.
    return std::abs(logicPos.x() - inRect.center().x()) <=
                   std::abs(logicPos.x() - outRect.center().x())
               ? mu::notation::LoopBoundaryType::LoopIn
               : mu::notation::LoopBoundaryType::LoopOut;
  if (inHit)
    return mu::notation::LoopBoundaryType::LoopIn;
  if (outHit)
    return mu::notation::LoopBoundaryType::LoopOut;
  return std::nullopt;
}

void OrchestrionNotationPaintView::dragLoopBoundaryTo(
    const muse::PointF &logicPos)
{
  const auto notation = this->notation();
  const auto masterNotation = globalContext()->currentMasterNotation();
  if (!notation || !masterNotation)
    return;

  const mu::engraving::Score *score = notation->elements()->msScore();
  mu::engraving::staff_idx_t staffIdx = 0;
  mu::engraving::Segment *segment = nullptr;
  score->pos2measure(logicPos, &staffIdx, nullptr, &segment, nullptr);
  if (!segment)
    return;

  // Snap to the chord under the cursor, and refuse to cross the other
  // boundary — the flag just stops at the last valid chord.
  const auto &boundaries = masterNotation->playback()->loopBoundaries();
  if (*m_draggedLoopBoundary == mu::notation::LoopBoundaryType::LoopIn)
  {
    const auto tick = segment->tick().ticks();
    if (tick != boundaries.loopInTick && tick < boundaries.loopOutTick)
      loopBoundariesController()->setLoopStart(tick);
  }
  else
  {
    const auto endTick = (segment->tick() + segment->ticks()).ticks();
    if (endTick != boundaries.loopOutTick && endTick > boundaries.loopInTick)
      loopBoundariesController()->setLoopEnd(endTick);
  }
}

void OrchestrionNotationPaintView::updateHoveredNoteInfo(const QPointF &itemPos)
{
  const auto interaction = notationInteraction();
  if (!interaction)
  {
    setHoveredNoteInfo({}, itemPos);
    return;
  }

  const mu::engraving::EngravingItem *const hitElement =
      interaction->hitElement(toLogical(itemPos), hitWidth());

  const mu::engraving::Chord *chord = nullptr;
  if (const auto note = dynamic_cast<const mu::engraving::Note *>(hitElement))
    chord = note->chord();
  else
    chord = dynamic_cast<const mu::engraving::Chord *>(hitElement);

  if (!chord)
  {
    setHoveredNoteInfo({}, itemPos);
    return;
  }

  // Map the hovered engraving chord back to its MuseChord. Matching on the
  // engraving-chord pointer disambiguates voices/staves sharing a segment.
  const IChord *museChord = nullptr;
  for (IMelodySegment *const segment : chordRegistry()->GetMelodySegments())
    if (IChord *const candidate = segment->AsChord();
        candidate && candidate->GetEngravingChord() == chord)
    {
      museChord = candidate;
      break;
    }

  if (!museChord)
  {
    setHoveredNoteInfo({}, itemPos);
    return;
  }

  const float dynamicVelocity = museChord->GetDynamicVelocity().value_or(0.f);
  setHoveredNoteInfo(
      QStringLiteral("dynamicVelocity: %1").arg(dynamicVelocity, 0, 'f', 3),
      itemPos);
}

void OrchestrionNotationPaintView::setHoveredNoteInfo(const QString &info,
                                                      const QPointF &itemPos)
{
  if (m_hoveredNoteInfo == info && m_hoveredNoteInfoPos == itemPos)
    return;
  m_hoveredNoteInfo = info;
  m_hoveredNoteInfoPos = itemPos;
  emit hoveredNoteInfoChanged();
}

QString OrchestrionNotationPaintView::hoveredNoteInfo() const
{
  return m_hoveredNoteInfo;
}

QPointF OrchestrionNotationPaintView::hoveredNoteInfoPos() const
{
  return m_hoveredNoteInfoPos;
}

float OrchestrionNotationPaintView::hitWidth() const
{
  return configuration()->selectionProximity() * 0.5f / currentScaling();
}

void OrchestrionNotationPaintView::wheelEvent(QWheelEvent *event)
{
  // A wheel/trackpad swipe (zoom or pan) is manual navigation: hand auto-scroll
  // control back to the user. An interruption also restarts the timing stats
  // once playing resumes, and ends the take: review time.
  m_follower.suspend();
  m_timingStatsStale = true;
  endTake();

  // Ctrl + wheel (or Ctrl + two-finger trackpad swipe, which Qt delivers as a
  // Ctrl-modified wheel event) zooms the score in/out about the cursor.
  if (event->modifiers() & Qt::ControlModifier)
  {
    zoomBy(*event);
    event->accept();
    return;
  }

  // The base class swallows wheel events (zoom + 2D scroll) because the
  // Orchestrion view controls its own scaling and keeps the single LINE-mode
  // system vertically centered. We re-enable just the one gesture we want:
  // a horizontal trackpad swipe pans the viewport left/right (with a kinetic
  // "throw"). Vertical scroll is ignored — there is nothing to scroll there.
  if (m_kineticScroller.handleWheelEvent(*event, width()))
    event->accept();
  else
    event->ignore();
}

void OrchestrionNotationPaintView::zoomBy(const QWheelEvent &event)
{
  // Mirrors mu::notation::NotationViewInputController::wheelEvent: turn the
  // wheel delta into "steps", then scale by zoomSpeed^steps about the cursor.
  // setScaling() runs constrainScorePosition() (via onMatrixChanged), so the
  // single LINE-mode system stays vertically centered after the zoom.
  QPoint pixels = event.pixelDelta();
  const QPoint angle = event.angleDelta();

#ifdef Q_OS_LINUX
  // pixelDelta is unreliable on X11; only trust it under Wayland (same caveat
  // as the base class and the KineticScroller).
  if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY"))
    pixels = QPoint{};
#endif

  // A mouse notch is one step; a trackpad reports finer pixel deltas.
  constexpr int pixelsPerStep = 5;
  qreal stepsX = 0.;
  qreal stepsY = 0.;
  if (!pixels.isNull())
  {
    stepsX = pixels.x() / static_cast<qreal>(pixelsPerStep);
    stepsY = pixels.y() / static_cast<qreal>(pixelsPerStep);
  }
  else if (!angle.isNull())
  {
    stepsX = angle.x() / static_cast<qreal>(QWheelEvent::DefaultDeltasPerStep);
    stepsY = angle.y() / static_cast<qreal>(QWheelEvent::DefaultDeltasPerStep);
  }

  const qreal steps = std::sqrt(stepsX * stepsX + stepsY * stepsY) *
                      (stepsY > -stepsX ? 1 : -1);
  if (qFuzzyIsNull(steps))
    return;

  const qreal zoomSpeed =
      std::pow(2.0, 1.0 / configuration()->mouseZoomPrecision());
  qreal scaling = currentScaling() * std::pow(zoomSpeed, steps);

  // Clamp to the zoom range MuseScore allows (its 5%–1600% list).
  const QList<int> zooms = configuration()->possibleZoomPercentageList();
  if (!zooms.isEmpty())
  {
    const qreal minScaling =
        configuration()->scalingFromZoomPercentage(zooms.first());
    const qreal maxScaling =
        configuration()->scalingFromZoomPercentage(zooms.last());
    scaling = std::clamp(scaling, minScaling, maxScaling);
  }

  setScaling(scaling, muse::PointF::fromQPointF(event.position()));
  // onMatrixChanged() records this as the user's new default zoom.
}

bool OrchestrionNotationPaintView::moveCanvasBy(qreal physicalDx)
{
  // The KineticScroller works in physical pixels; convert to the score's
  // logical units. constrainScorePosition() (via onMatrixChanged) clamps the
  // result, so at an edge the viewport doesn't move and we report that back to
  // stop the glide.
  const qreal before = viewport().left();
  moveCanvasHorizontal(physicalDx / currentScaling());
  return qAbs(viewport().left() - before) > 1e-3;
}

void OrchestrionNotationPaintView::loadOrchestrionNotation()
{
  qApp->installEventFilter(this);

  // Drive the follow from the window's frame loop rather than from its own
  // timer (see ScoreFollower::frameTick).
  connect(this, &QQuickItem::windowChanged, this,
          &OrchestrionNotationPaintView::connectFrameTick);
  connectFrameTick(window());

  // MuseScore pans the canvas to keep *its* playback cursor in view whenever
  // its playback position changes. Orchestrion doesn't use that playhead at
  // all — the follower owns the scroll — so the two fight: every position
  // change yanked the canvas back to wherever MuseScore's cursor sat (near
  // the score start), and our next follow tick, up to 16 ms later, yanked it
  // back. One frame at the wrong x, i.e. a visible jerk, per position change.
  configuration()->setIsAutomaticallyPanEnabled(false);

  orchestrion()->sequencerChanged().onNotify(
      this,
      [this]
      {
        const auto sequencer = orchestrion()->sequencer();
        const auto registry = orchestrion()->modifiableItemRegistry();
        if (!sequencer || !registry)
          return;
        subscribe(*sequencer, *registry);
      });
  const auto sequencer = orchestrion()->sequencer();
  const auto registry = orchestrion()->modifiableItemRegistry();
  if (sequencer && registry)
    subscribe(*sequencer, *registry);

  globalContext()->currentNotationChanged().onNotify(
      this,
      [this]
      {
        AbstractNotationPaintView::onNotationSetup();
        updateNotation();
      });

  sequencerConfiguration()->noteInfoTooltipEnabledChanged().onNotify(
      this,
      [this]
      {
        if (!sequencerConfiguration()->noteInfoTooltipEnabled())
          setHoveredNoteInfo({}, m_hoveredNoteInfoPos);
      });

  sequencerConfiguration()->tempoVisualizationEnabledChanged().onNotify(
      this, [this] { emit tempoVisualizationEnabledChanged(); });

  m_estimator.setMemory(sequencerConfiguration()->tempoSmoothingMemory());

  // Re-arm (or disarm) the finished take when the play mode changes. A
  // half-recorded take stays unarmed; endTake will push it with the new mode.
  orchestrion()->playModeChanged().onNotify(this,
                                            [this]
                                            {
                                              if (m_takeOver)
                                                pushReplayTake();
                                            });

  m_follower.setAnticipateJumps(
      sequencerConfiguration()->jumpAnticipationEnabled());
  sequencerConfiguration()->jumpAnticipationEnabledChanged().onNotify(
      this,
      [this]
      {
        m_follower.setAnticipateJumps(
            sequencerConfiguration()->jumpAnticipationEnabled());
      });

  m_timingOverlay.setPersistent(
      sequencerConfiguration()->persistentTimingMarksEnabled());
  sequencerConfiguration()->persistentTimingMarksEnabledChanged().onNotify(
      this,
      [this]
      {
        m_timingOverlay.setPersistent(
            sequencerConfiguration()->persistentTimingMarksEnabled());
        update();
      });

  sequencerConfiguration()->handSyncScoreEnabledChanged().onNotify(
      this,
      [this]
      {
        // Toggled off: stale sync samples shouldn't linger in the verdict.
        // (Toggling on starts collecting from here on, nothing to do.)
        if (!sequencerConfiguration()->handSyncScoreEnabled())
          m_timingOverlay.clearSyncStats();
        update();
      });

  sequencerConfiguration()->dynamicsScoreEnabledChanged().onNotify(
      this,
      [this]
      {
        if (!sequencerConfiguration()->dynamicsScoreEnabled())
          m_timingOverlay.clearDynamicsStats();
        update();
      });

  // Auto-play: the ledger is rebuilt from scratch when the chosen hand
  // changes (or when the feature is hidden/exposed), seeded from whatever the
  // current batch holds — after loading it covers every voice; mid-piece it
  // may not, and a rewind repopulates it in full.
  const auto configureAutoPlay = [this]
  {
    m_autoTrackTargets.clear();
    m_autoOffTick.reset();
    m_autoOnTick.reset();
    m_autoPlayTimer.stop();
    if (const auto sequencer = orchestrion()->sequencer())
      updateAutoTargets(sequencer->GetCurrentTransitions());
  };
  configureAutoPlay();
  sequencerConfiguration()->autoPlayedStaffChanged().onNotify(
      this, configureAutoPlay);
  sequencerConfiguration()->autoPlayExposedChanged().onNotify(
      this, configureAutoPlay);

  // Toggling the layout mode re-lays-out the score; every cached x is stale,
  // which is exactly what updateNotation() resets (follower, stats, marks).
  // The shadow copies only mean anything on the time-proportional canvas.
  // The grading master switch gets the same treatment: turning
  // it off must return the score to plain engraving and clear every mark.
  const auto applyGradingMode = [this]
  {
    m_timingOverlay.setShadowsEnabled(
        sequencerConfiguration()->gradingEnabled() &&
        sequencerConfiguration()->timeProportionalSpacingEnabled());
    updateNotation();
  };
  m_timingOverlay.setShadowsEnabled(
      sequencerConfiguration()->gradingEnabled() &&
      sequencerConfiguration()->timeProportionalSpacingEnabled());
  sequencerConfiguration()->timeProportionalSpacingEnabledChanged().onNotify(
      this, applyGradingMode);
  sequencerConfiguration()->gradingEnabledChanged().onNotify(this,
                                                             applyGradingMode);

  load();
  updateNotation();

  const auto interaction = notationInteraction();
  IF_ASSERT_FAILED(interaction) { return; }
  interaction->noteInput()->stateChanged().onNotify(
      this,
      [this]
      {
        QTimer::singleShot(0, this,
                           [this]
                           {
                             // Same as above: restore this to `true` in case
                             // the base class has set it to `false`.
                             setAcceptHoverEvents(true);
                           });
      },
      AsyncMode::AsyncSetRepeat);
}

void OrchestrionNotationPaintView::onMatrixChanged(
    const muse::draw::Transform &oldMatrix,
    const muse::draw::Transform &newMatrix, bool overrideZoomType)
{
  NotationPaintView::onMatrixChanged(oldMatrix, newMatrix, overrideZoomType);

  // Any canvas move we didn't drive ourselves — a drag, a swipe, a zoom, a
  // relayout — moved the follower's page from under it: let it pick the page
  // up where it now is.
  if (!m_drivingScroll)
    m_follower.viewMoved();

  // A zoom (wheel, pinch, keyboard, toolbar, ...) is the user's choice — the
  // follower never zooms — and manual navigation: hand control back until
  // they play again.
  if (!m_drivingScroll && !qFuzzyCompare(oldMatrix.m11(), newMatrix.m11()))
  {
    m_follower.suspend();
    m_timingStatsStale = true;
    endTake();
  }

  constrainScorePosition();
}

void OrchestrionNotationPaintView::updateNotation()
{
  m_kineticScroller.stop(); // the score changed under us; cancel any glide
  m_follower.reset();       // and the follow state
  m_readingFocus.clear();
  m_estimator.reset(); // and the position estimate
  m_loudness.clear();
  m_tempoVizModel.clear();
  if (const auto notation = globalContext()->currentNotation())
  {
    // Time-proportional spacing uses MuseScore's duration-proportional
    // layout (with the fork's global quantum), so equal horizontal distance
    // = equal musical time — the canvas for the tempo-warped note overlays.
    // It only serves the grading: without it, plain engraving.
    setViewMode(
        sequencerConfiguration()->gradingEnabled() &&
                sequencerConfiguration()->timeProportionalSpacingEnabled()
            ? mu::notation::ViewMode::HORIZONTAL_FIXED
            : mu::notation::ViewMode::LINE);
    auto config = notation->interaction()->scoreConfig();
    config.isShowInvisibleElements = false;
    config.isShowUnprintableElements = false;
    config.isShowFrames = false;
    config.isShowPageMargins = false;
    config.isShowSoundFlags = false;
    notation->interaction()->setScoreConfig(config);
    constrainScorePosition();
  }
  m_boxes.clear();
  m_fader.clear();
  // A different score: the error stats start over immediately.
  m_timingOverlay.reset();
  m_timingStatsStale = false;
  m_finalScoreShown = false;
  m_takeOnsetRecords.clear();
  m_replayEvents.clear();
  m_replayStartTick = std::numeric_limits<int>::max();
  m_takeOver = false;
  orchestrion()->player()->SetReplayTake(std::nullopt);
  clearPerformanceWarp();
  dismissFinalScore();
  emit smoothingTunerVisibleChanged();
  update();
}

void OrchestrionNotationPaintView::setViewMode(mu::notation::ViewMode mode)
{
  const auto notation = this->notation();
  if (!notation)
    return;
  notation->viewState()->setViewMode(mode);
  notation->painting()->setViewMode(mode);
}

void OrchestrionNotationPaintView::constrainScorePosition()
{
  // While the follow logic is placing the canvas it owns the position (and
  // centers the system vertically itself); don't fight it.
  if (m_drivingScroll)
    return;

  // moveCanvasToPosition() below feeds back into this function via
  // onMatrixChanged(). Usually that re-entrant call lands on the same position
  // and stops, but at very low zoom our constraint and the base class's canvas
  // constraint disagree and never reach a common fixed point, so the recursion
  // overflows the stack. Guard against re-entry — a single pass is enough.
  if (m_constrainingScorePosition)
    return;
  m_constrainingScorePosition = true;

  const auto content = notationContentRect(); // logical
  const auto scaling = currentScaling();
  const auto emptyAbovePhysical = (height() - content.height() * scaling) / 2.;
  const auto topLogicalY = content.top() - emptyAbovePhysical / scaling;

  const double leftLogicalX = clampLeftX(viewport().left(), scaling);

  moveCanvasToPosition(muse::PointF{leftLogicalX, topLogicalY});

  m_constrainingScorePosition = false;
}

std::optional<OrchestrionNotationPaintView::BarrierAhead>
OrchestrionNotationPaintView::nextBarrier(int utick) const
{
  const auto notation = this->notation();
  if (!notation)
    return std::nullopt;
  const mu::engraving::Score *score = notation->elements()->msScore();
  if (!score)
    return std::nullopt;
  // The expanded list is the one the sequencer unrolled the score with (see
  // OrchestrionSequencerFactory), so its uticks are the chords'.
  const mu::engraving::RepeatList &repeats = score->repeatList(true);
  for (std::size_t i = 0; i < repeats.size(); ++i)
  {
    const mu::engraving::RepeatSegment *segment = repeats[i];
    if (utick >= segment->utick + segment->len())
      continue; // not there yet
    // The reading carries on from one unrolled segment into the next as long
    // as the next starts at the score tick where this one ends (the list is
    // also cut at repeat starts and voltas that are simply played through).
    while (i + 1 < repeats.size() &&
           repeats[i + 1]->tick == segment->tick + segment->len())
      segment = repeats[++i];
    const mu::engraving::Measure *last = segment->lastMeasure();
    if (!last)
      return std::nullopt;
    BarrierAhead ahead;
    ahead.barrier.x = last->pageBoundingRect().right();
    ahead.barrier.utick = segment->utick + segment->len();
    if (i + 1 >= repeats.size())
      return ahead; // the final barline: nowhere to resume
    // Where the reading resumes: the start of the next unrolled segment.
    const mu::engraving::RepeatSegment *next = repeats[i + 1];
    const mu::engraving::Measure *first = next->firstMeasure();
    if (!first)
      return ahead;
    ahead.barrier.resumeX = first->pageBoundingRect().left();
    // The first note after the jump: the first chord (any track) in the
    // segment resumed at — or, failing one, the jump itself.
    ahead.resumeUtick = next->utick;
    for (const mu::engraving::Measure *measure : next->measureList())
      for (const mu::engraving::Segment *seg =
               measure->first(mu::engraving::SegmentType::ChordRest);
           seg; seg = seg->next(mu::engraving::SegmentType::ChordRest))
        for (const mu::engraving::EngravingItem *el : seg->elist())
          if (el && el->isChord())
          {
            ahead.resumeUtick = next->utick + seg->tick().ticks() - next->tick;
            return ahead;
          }
    return ahead;
  }
  return std::nullopt;
}

std::optional<double> OrchestrionNotationPaintView::resumeExpectedInMs() const
{
  if (!m_resumeUtick)
    return std::nullopt;
  const double nowMs = static_cast<double>(m_clock.elapsed());
  constexpr double ticksPerQuarter = mu::engraving::Constants::DIVISION;
  std::optional<double> expectedInMs;
  for (int staff = 0; staff < 2; ++staff)
  {
    // A hand not yet tracked has no say; one coasting — its next note
    // overdue, winding down — has none either: it would only extrapolate a
    // tempo it has stopped keeping.
    if (!m_estimator.ready(staff) || m_estimator.isCoasting(staff))
      continue;
    std::optional<double> position = m_estimator.tickAt(staff, nowMs);
    const std::optional<double> bpm = m_estimator.bpm(staff);
    if (!position || !bpm || *bpm <= 1.0)
      continue;
    // Not past the next note to play (see the declaration).
    if (m_focusUtick)
      position = std::min(*position, static_cast<double>(*m_focusUtick));
    const double ticksPerMs = *bpm * ticksPerQuarter / 60000.0;
    const double ms = (*m_resumeUtick - *position) / ticksPerMs;
    expectedInMs = expectedInMs ? std::min(*expectedInMs, ms) : ms;
  }
  return expectedInMs;
}

double OrchestrionNotationPaintView::clampLeftX(double desiredLeftX,
                                                double scaling) const
{
  // Two horizontal rules (shared by the manual constraint and the auto-follow):
  // 1. not more than maxEmptyPhysical empty pixels past either end of the
  // system;
  // 2. if the system is narrower than the view, it stays centered.
  const auto content = notationContentRect();
  constexpr double maxEmptyPhysical = 200.;
  const double contentWidthPhysical = content.width() * scaling;
  if (contentWidthPhysical < width())
    return content.left() - (width() - contentWidthPhysical) / (2 * scaling);
  const double minLeft = content.left() - maxEmptyPhysical / scaling;
  const double maxLeft =
      content.right() + maxEmptyPhysical / scaling - width() / scaling;
  return std::clamp(desiredLeftX, minLeft, maxLeft);
}

void OrchestrionNotationPaintView::paintBeatLines(QPainter *painter)
{
  if (!m_takeOver || m_takeOnsetRecords.size() < 2)
    return;

  // The onsets' (utick, live x): the anchors are the engraved elements, so
  // the grid follows the warp morph and the γ-slider re-fits. Between
  // onsets, both the utick→x layout and the utick→time fit are linear, so
  // beats interpolate exactly.
  std::vector<std::pair<double, double>> points;
  points.reserve(m_takeOnsetRecords.size());
  for (const TakeOnsetRecord &record : m_takeOnsetRecords)
    if (record.anchor)
      points.emplace_back(record.utick,
                          record.anchor->pageBoundingRect().center().x());
  std::sort(points.begin(), points.end());
  if (points.size() < 2)
    return;

  const QRectF view = viewport().toQRectF();
  painter->save();
  QPen pen(QColor(90, 43, 37, 60)); // faint mahogany hairline
  pen.setWidthF(0.0);
  pen.setCosmetic(true);
  painter->setPen(pen);

  auto lower = points.begin();
  for (double beat =
           std::ceil(points.front().first / beatGridTicks) * beatGridTicks;
       beat <= points.back().first; beat += beatGridTicks)
  {
    while (lower + 1 != points.end() && (lower + 1)->first <= beat)
      ++lower;
    if (lower + 1 == points.end())
      break;
    const auto &[u0, x0] = *lower;
    const auto &[u1, x1] = *(lower + 1);
    // x jumping backwards = a repeat seam: no grid across the page jump.
    if (u1 <= u0 || x1 < x0)
      continue;
    const double x = x0 + (beat - u0) / (u1 - u0) * (x1 - x0);
    painter->drawLine(QPointF(x, view.top()), QPointF(x, view.bottom()));
  }
  painter->restore();
}

void OrchestrionNotationPaintView::paintNotationUnderlay(QPainter *painter)
{
  // Called by the base view once the painter carries the score's world
  // transform and before the notation is drawn — so the highlight sits behind
  // the notes (but on top of the background), and we draw in logical
  // coordinates.
  paintLoopRegionUnderlay(painter);
  paintBeatLines(painter);

  if (m_boxes.empty() && m_fader.empty())
    return;

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(Qt::NoPen);
  painter->setOpacity(1.0);

  // Soft highlighter-style fill: a translucent rounded block behind the notes.
  // `opacity` scales the strength (1 while ringing/upcoming, ramping to 0 as a
  // just-ended note's highlight fades out).
  const auto fillBox = [painter](const Highlight &box, double opacity)
  {
    const QRectF &rect = box.rect;
    QColor fill = box.color;
    fill.setAlphaF(box.intensity * 0.3 * opacity);
    painter->setBrush(fill);
    qreal r = box.spatium * 1.5;
    r = std::min(r, rect.width() / 2);
    r = std::min(r, rect.height() / 2);
    painter->drawRoundedRect(rect, r, r);
  };

  for (const auto &entry : m_boxes)
    fillBox(entry.second, 1.0);

  m_fader.forEach(fillBox);
  painter->restore();
}

namespace
{
// Orchestrion loop-marker palette: the wallpaper's dark espresso for the
// handles and the region shading, the cream accent (Theme.accent) for the dot.
const QColor loopHandleColor{0x3C, 0x1F, 0x19};
const QColor loopAccentColor{0xF0, 0xE5, 0xC8};
} // namespace

void OrchestrionNotationPaintView::paintLoopRegionUnderlay(QPainter *painter)
{
  const auto masterNotation = globalContext()->currentMasterNotation();
  if (!masterNotation || !masterNotation->playback()->loopBoundaries().enabled)
    return;

  const auto inRect = loopInMarkerRect();
  const auto outRect = loopOutMarkerRect();
  // Shade only the simple case of both boundaries on the same system — always
  // true in Orchestrion's horizontal continuous view.
  if (inRect.isEmpty() || outRect.isEmpty() ||
      std::abs(inRect.top() - outRect.top()) > .5 ||
      outRect.left() <= inRect.left())
    return;

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(Qt::NoPen);
  // The score band is itself cream, so shade the looped span with a whisper
  // of the handles' espresso instead of the accent color.
  QColor tint = loopHandleColor;
  tint.setAlpha(28);
  painter->setBrush(tint);
  const QRectF region{QPointF{inRect.left(), inRect.top()},
                      QPointF{outRect.left(), inRect.bottom()}};
  painter->drawRect(region);
  painter->restore();
}

void OrchestrionNotationPaintView::paintLoopMarkers(
    muse::draw::Painter *painter)
{
  const auto notation = this->notation();
  const auto masterNotation = globalContext()->currentMasterNotation();
  if (!notation || !masterNotation ||
      !masterNotation->playback()->loopBoundaries().enabled)
    return;

  const double spatium =
      notation->style()->styleValue(mu::notation::StyleId::spatium).toDouble();
  if (spatium <= 0)
    return;

  const auto paintHandle = [&](const muse::RectF &rect, bool isLoopIn)
  {
    if (rect.isEmpty())
      return;

    painter->setNoPen();
    painter->setAntialiasing(true);
    painter->setBrush(loopHandleColor);

    // A slim vertical pill spanning the system...
    const double barWidth = 0.45 * spatium;
    const double x = rect.left();
    const muse::RectF bar{x - barWidth / 2, rect.top(), barWidth,
                          rect.height()};
    painter->drawRoundedRect(bar, barWidth / 2, barWidth / 2);

    // ...with a rounded tab at the top pointing into the loop...
    const double tabWidth = 1.9 * spatium;
    const double tabHeight = 1.4 * spatium;
    const muse::RectF tab{isLoopIn ? x - barWidth / 2
                                   : x + barWidth / 2 - tabWidth,
                          rect.top(), tabWidth, tabHeight};
    painter->drawRoundedRect(tab, 0.5 * spatium, 0.5 * spatium);

    // ...and the cream accent dot on the tab.
    painter->setBrush(loopAccentColor);
    const double dotRadius = 0.32 * spatium;
    painter->drawEllipse(tab.center(), dotRadius, dotRadius);
  };

  paintHandle(loopInMarkerRect(), true);
  paintHandle(loopOutMarkerRect(), false);
}

void OrchestrionNotationPaintView::paint(QPainter *painter)
{
  NotationPaintView::paint(painter);

  painter->setRenderHint(QPainter::Antialiasing);
  painter->setBrush(Qt::NoBrush);
  painter->setOpacity(1.0);

  const auto view = viewport();

  // Timing-judgment overlay (gauges next to the notes + box-plot HUD), on top
  // of the notation.
  if (sequencerConfiguration()->gradingEnabled())
    m_timingOverlay.paint(*painter, view.toQRectF(), currentScaling());
}
} // namespace dgk