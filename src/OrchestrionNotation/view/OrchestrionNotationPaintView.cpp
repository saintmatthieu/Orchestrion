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
#include "GestureControllers/ITouchpadGestureController.h"
#include "OrchestrionSequencer/IChord.h"
#include "OrchestrionSequencer/IOrchestrionSequencer.h"
#include "OrchestrionSequencer/IRest.h"
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QWheelEvent>
#include <engraving/dom/chord.h>
#include <engraving/dom/masterscore.h>
#include <engraving/dom/measure.h>
#include <engraving/dom/note.h>
#include <engraving/dom/segment.h>
#include <engraving/dom/system.h>
#include <engraving/dom/tie.h>
#include <notation/imasternotation.h>

#include <cmath>
#include <optional>

namespace dgk
{
namespace
{
// The sequencer routes input events to hands by pitch (< 60 = left hand, see
// OrchestrionSequencer::OnInputEventRecursive) — the same sentinels the
// automatic player uses.
constexpr int rightHandPitch = 60;
constexpr int leftHandPitch = 59;

// The warp-bake animation: the score morphs from ideal to performed spacing
// (each step is a full horizontal relayout — cheap in linear view mode).
constexpr int warpAnimSteps = 18;
constexpr int warpAnimStepMs = 40;
} // namespace

OrchestrionNotationPaintView::OrchestrionNotationPaintView(QQuickItem *parent)
    : mu::notation::NotationPaintView(parent), m_fader([this] { update(); }),
      m_timingOverlay([this] { update(); }),
      m_follower(*this, &m_tempoVizModel),
      m_kineticScroller([this](qreal physicalDx)
                        { return moveCanvasBy(physicalDx); })
{
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
                                              m_follower.reset();
                                              m_tempoVizModel.clear();
                                              m_boxes.clear();
                                              m_fader.clear();
                                              // An interruption: the timing
                                              // stats restart when playing
                                              // resumes (readable until then).
                                              m_timingStatsStale = true;
                                              bakePerformanceWarp();
                                              // The jump's transitions batch
                                              // repopulates the ledger.
                                              m_autoTrackTargets.clear();
                                              update();
                                            });
}

void OrchestrionNotationPaintView::OnTransitions(
    const std::map<TrackIndex, ChordTransition> &transitions)
{
  // Onsets driving the follow, grouped per hand (= staff): the voices on a
  // staff are played by the same gestures, so they share one tracker. Each
  // hand's sounding onset is a tempo observation for it; the leading/trailing
  // of all onsets (sounding or upcoming) feed the one-shot initial framing.
  std::map<int /*staff*/, TempoFollower::Onset> presentOnsets;
  std::map<int /*staff*/, int> presentScoreTicks; // engraved position
  std::optional<double> leadingAnyX;
  std::optional<double> trailingAnyX;
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

    // Onset x of this track's note (page-logical), for the follow. Use the
    // segment element rather than the hugging box, whose width includes ties.
    if (const auto el = segment->element(track.value))
    {
      const double onsetX = el->pageBoundingRect().center().x();
      leadingAnyX = leadingAnyX ? std::max(*leadingAnyX, onsetX) : onsetX;
      trailingAnyX = trailingAnyX ? std::min(*trailingAnyX, onsetX) : onsetX;
      if (active)
      {
        // Collapse a staff's voices into one onset (its rightmost), carrying
        // the playback-unrolled tick — continuous through repeats, voltas and
        // jumps — for the musical-tempo readout and the timing judgments.
        const int hand = track.staffIndex();
        // The gesture's controller velocity (cached from HandNoteEvents just
        // before this batch), for the dynamics judgments.
        const std::optional<float> &velocity =
            m_pendingHandVelocity[hand > 0 ? 1 : 0];
        const auto it = presentOnsets.find(hand);
        if (it == presentOnsets.end() || onsetX > it->second.x)
        {
          presentOnsets[hand] = TempoFollower::Onset{
              onsetX, static_cast<double>(present->GetBeginTick().withRepeats),
              velocity ? std::make_optional<double>(*velocity) : std::nullopt};
          presentScoreTicks[hand] = segment->tick().ticks();
        }
      }
    }
  }

  const TempoFollower::Feedback feedback =
      m_follower.onOnsets(presentOnsets, leadingAnyX, trailingAnyX);
  // The cached velocities were for this batch only.
  m_pendingHandVelocity[0].reset();
  m_pendingHandVelocity[1].reset();

  // Playing has resumed after an interruption: the error stats start a fresh
  // take. (It stays readable while interrupted; only the resume clears it.)
  if (m_timingStatsStale && !presentOnsets.empty())
  {
    m_timingOverlay.reset();
    m_timingStatsStale = false;
    m_finalScoreShown = false;
    m_takeOnsetRecords.clear();
    clearPerformanceWarp();
    dismissFinalScore();
  }

  // The game feedback: an error gauge next to the struck notes — above the
  // staff for the right hand, below for the left — and the hand's revised
  // judgments into the overlay (moving still-showing markers, re-binning the
  // box plot). The newest onset's judgment is the window's last.
  if (sequencerConfiguration()->timingFeedbackEnabled())
  {
    for (const auto &[staff, window] : feedback.judgments)
    {
      if (window.empty())
        continue;
      if (const auto it = staffHits.find(staff); it != staffHits.end())
      {
        const StaffHit &hit = it->second;
        const bool below = staff > 0; // left hand
        m_timingOverlay.addGauge(staff, window.back().tMs, hit.rect,
                                 hit.spatium, window.back().errorMs, below,
                                 below ? hit.staffBottom : hit.staffTop,
                                 hit.items);
      }
      if (const auto tickIt = presentScoreTicks.find(staff);
          tickIt != presentScoreTicks.end())
        m_takeOnsetRecords.push_back({staff, window.back().tMs,
                                      tickIt->second,
                                      presentOnsets.at(staff).tick});
      m_timingOverlay.updateJudgments(staff, window);
    }
    if (feedback.handSync && sequencerConfiguration()->handSyncScoreEnabled())
      m_timingOverlay.addSyncSample(feedback.handSync->tMs,
                                    feedback.handSync->errorMs);
    if (sequencerConfiguration()->dynamicsScoreEnabled())
      for (const auto &[staff, window] : feedback.dynamicsJudgments)
        m_timingOverlay.updateDynamicsJudgments(staff, window);
  }

  // End of the piece: nothing is sounding (notes *and* rests) and nothing is
  // upcoming on any voice — the last notes were just released — so raise the
  // final-score banner, once. (While a chord sounds or a rest passes, its
  // transition is a *present* state carrying no future, so both must be
  // absent to distinguish the true end; a mid-piece release holds a future
  // chord or a present rest instead.)
  if (!m_finalScoreShown && sequencerConfiguration()->timingFeedbackEnabled())
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
        if (const auto score = m_timingOverlay.takeFinalScore())
        {
          m_finalScoreShown = true;
          m_finalScore = *score;
          m_finalScoreBreakdown = m_timingOverlay.takeScoreBreakdown();
          emit finalScoreChanged();
        }
        // The take is over: morph the page onto its fitted tempo curve.
        bakePerformanceWarp();
      }
    }

  updateAutoTargets(transitions);
}

void OrchestrionNotationPaintView::updateAutoTargets(
    const std::map<TrackIndex, ChordTransition> &batch)
{
  const int autoStaff = sequencerConfiguration()->autoPlayedStaff();
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
        present
            ? std::make_optional<double>(present->GetEndTick().withRepeats)
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
  m_follower.setAutoTargets(offTick, onTick);
}

void OrchestrionNotationPaintView::bakePerformanceWarp()
{
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
  m_warpProgress = 0.0;
  m_warpTimer.start();
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
  const auto master =
      masterNotation ? masterNotation->masterScore() : nullptr;
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
  const auto master =
      masterNotation ? masterNotation->masterScore() : nullptr;
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
  emit finalScoreChanged();
}

double OrchestrionNotationPaintView::minScaling() const
{
  const QList<int> zooms = configuration()->possibleZoomPercentageList();
  if (zooms.isEmpty())
    return currentScaling() * 0.1;
  return configuration()->scalingFromZoomPercentage(zooms.first());
}

void OrchestrionNotationPaintView::centerOn(double logicalX, double scaling)
{
  // constrainScorePosition() (via onMatrixChanged) would otherwise pull the
  // viewport back to hug the content; yield to us while we place the canvas.
  m_drivingScroll = true;

  if (!qFuzzyCompare(currentScaling(), scaling))
    setScaling(scaling, muse::PointF{0., 0.});

  const double logicalWidth = width() / scaling;
  // Rest the anchor at the follower's playhead fraction, but never past the
  // max-padding limit (so near the start/end of the score the anchor drifts
  // off its spot rather than opening a gap wider than a manual zoom-out would
  // allow).
  const double leftX =
      clampLeftX(logicalX - TempoFollower::anchorFrac * logicalWidth, scaling);

  const auto content = notationContentRect();
  const double emptyAbovePhysical =
      (height() - content.height() * scaling) / 2.;
  const double topY = content.top() - emptyAbovePhysical / scaling;

  moveCanvasToPosition(muse::PointF{leftX, topY});

  m_drivingScroll = false;
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
  bakePerformanceWarp();     // an interruption ends the take: review time
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
  if (sequencerConfiguration()->timingFeedbackEnabled())
  {
    const QPointF logical(logicPos.x(), logicPos.y());
    timingInfo = m_timingOverlay.gaugeInfoAt(logical);
    if (timingInfo.isEmpty())
      timingInfo = m_timingOverlay.ribbonInfoAt(logical);
  }
  if (!timingInfo.isEmpty())
    setHoveredNoteInfo(timingInfo, pos);
  else if (sequencerConfiguration()->noteInfoTooltipEnabled())
    updateHoveredNoteInfo(pos);
  else if (!m_hoveredNoteInfo.isEmpty())
    setHoveredNoteInfo({}, pos);
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
  bakePerformanceWarp();

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

  gestureControllerSelector()->touchpadControllerChanged().onNotify(
      this, [this] { initTouchpadMidiController(); });
  initTouchpadMidiController();

  sequencerConfiguration()->noteInfoTooltipEnabledChanged().onNotify(
      this,
      [this]
      {
        if (!sequencerConfiguration()->noteInfoTooltipEnabled())
          setHoveredNoteInfo({}, m_hoveredNoteInfoPos);
      });

  sequencerConfiguration()->tempoVisualizationEnabledChanged().onNotify(
      this, [this] { emit tempoVisualizationEnabledChanged(); });

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

  // Tempo-following auto-play: the follower fires when the manual hands'
  // estimate reaches the auto hand's next due point; we inject the hand event
  // (deferred out of the follow tick — the resulting transitions re-enter it
  // with fresh targets).
  const auto configureAutoPlay = [this]
  {
    const int staff = sequencerConfiguration()->autoPlayedStaff();
    m_autoTrackTargets.clear();
    m_follower.setAutoPlay(
        staff < 0 ? std::nullopt : std::make_optional(staff),
        [this](bool noteOn)
        {
          QTimer::singleShot(
              0, this,
              [this, noteOn]
              {
                const int staff = sequencerConfiguration()->autoPlayedStaff();
                const auto sequencer = orchestrion()->sequencer();
                if (staff < 0 || !sequencer)
                  return;
                sequencer->OnInputEvent(noteOn ? NoteEventType::noteOn
                                               : NoteEventType::noteOff,
                                        staff > 0 ? leftHandPitch
                                                  : rightHandPitch,
                                        std::nullopt);
              });
        });
    // Seed from whatever the last batch holds (after loading it covers every
    // voice; mid-piece it may not — a rewind repopulates it in full).
    if (const auto sequencer = orchestrion()->sequencer())
      updateAutoTargets(sequencer->GetCurrentTransitions());
  };
  configureAutoPlay();
  sequencerConfiguration()->autoPlayedStaffChanged().onNotify(
      this, configureAutoPlay);

  // Toggling the layout mode re-lays-out the score; every cached x is stale,
  // which is exactly what updateNotation() resets (follower, stats, marks).
  // The shadow copies only mean anything on the time-proportional canvas.
  m_timingOverlay.setShadowsEnabled(
      sequencerConfiguration()->timeProportionalSpacingEnabled());
  sequencerConfiguration()->timeProportionalSpacingEnabledChanged().onNotify(
      this,
      [this]
      {
        m_timingOverlay.setShadowsEnabled(
            sequencerConfiguration()->timeProportionalSpacingEnabled());
        updateNotation();
      });

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

void OrchestrionNotationPaintView::initTouchpadMidiController()
{
  const auto touchpad = gestureControllerSelector()->getTouchpadController();

  // set cursor to invisible if touchpad is not null:
  if (touchpad)
    setCursor(Qt::BlankCursor);
  else
    setCursor(Qt::ArrowCursor);

  if (!touchpad)
    return;
  touchpad->contactChanged().onReceive(
      this,
      [this](const Contacts &contacts)
      {
        // Delete contacts that are no longer active:
        for (auto it = m_contacts.begin(); it != m_contacts.end();)
        {
          if (std::find_if(contacts.begin(), contacts.end(),
                           [&](const auto &entry) {
                             return entry.uid == it->first;
                           }) == contacts.end())
            it = m_contacts.erase(it);
          else
            ++it;
        }

        for (const auto &contact : contacts)
        {
          if (m_contacts.find(contact.uid) == m_contacts.end())
            m_contacts.emplace(contact.uid,
                               Contact{contact.x < 0.5, contact.x, contact.y});
          else
          {
            m_contacts.at(contact.uid).x = contact.x;
            m_contacts.at(contact.uid).y = contact.y;
          }
        }

        update();
      });
}

void OrchestrionNotationPaintView::onMatrixChanged(
    const muse::draw::Transform &oldMatrix,
    const muse::draw::Transform &newMatrix, bool overrideZoomType)
{
  NotationPaintView::onMatrixChanged(oldMatrix, newMatrix, overrideZoomType);

  // A zoom we didn't drive ourselves (wheel, pinch, keyboard, toolbar, ...) is
  // the user's choice: adopt it as the new default the auto-zoom won't exceed,
  // and hand control back until they play again.
  if (!m_drivingScroll && !qFuzzyCompare(oldMatrix.m11(), newMatrix.m11()))
  {
    m_userDefaultScaling = newMatrix.m11();
    m_follower.suspend();
    m_timingStatsStale = true;
    bakePerformanceWarp();
  }

  constrainScorePosition();
}

void OrchestrionNotationPaintView::updateNotation()
{
  m_kineticScroller.stop(); // the score changed under us; cancel any glide
  m_follower.reset();       // and the tempo estimate / follow state
  m_tempoVizModel.clear();
  if (const auto notation = globalContext()->currentNotation())
  {
    // Time-proportional spacing uses MuseScore's duration-proportional
    // layout (with the fork's global quantum), so equal horizontal distance
    // = equal musical time — the canvas for the tempo-warped note overlays.
    setViewMode(sequencerConfiguration()->timeProportionalSpacingEnabled()
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
    // The fit zoom after layout is the user's default until they change it.
    m_userDefaultScaling = currentScaling();
  }
  m_boxes.clear();
  m_fader.clear();
  // A different score: the error stats start over immediately.
  m_timingOverlay.reset();
  m_timingStatsStale = false;
  m_finalScoreShown = false;
  m_autoTrackTargets.clear();
  m_takeOnsetRecords.clear();
  clearPerformanceWarp();
  dismissFinalScore();
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

void OrchestrionNotationPaintView::paintNotationUnderlay(QPainter *painter)
{
  // Called by the base view once the painter carries the score's world
  // transform and before the notation is drawn — so the highlight sits behind
  // the notes (but on top of the background), and we draw in logical
  // coordinates.
  paintLoopRegionUnderlay(painter);

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

  painter->restore();
  // Touch contacts stay on top of everything.
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setBrush(Qt::NoBrush);
  painter->setOpacity(1.0);

  const auto view = viewport();

  // Timing-judgment overlay (gauges next to the notes + box-plot HUD), on top
  // of the notation.
  if (sequencerConfiguration()->timingFeedbackEnabled())
    m_timingOverlay.paint(*painter, view.toQRectF(), currentScaling());

  const auto radius = 30. / currentScaling();

  painter->setPen(Qt::NoPen);
  painter->setOpacity(0.1);

  std::for_each(m_contacts.begin(), m_contacts.end(),
                [&](const std::pair<int, Contact> &entry)
                {
                  const Contact &contact = entry.second;
                  painter->setBrush(contact.isLeft ? Qt::red : Qt::blue);
                  // Opacity 0.5
                  // Draw a circle, mapping normalized x and y to the viewport
                  // (physical coordinates)
                  const auto x = view.left() + contact.x * view.width();
                  const auto y = view.top() + contact.y * view.height();
                  painter->drawEllipse(QPointF{x, y}, radius, radius);
                });
}
} // namespace dgk