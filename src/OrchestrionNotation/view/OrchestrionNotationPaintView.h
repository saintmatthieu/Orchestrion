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
#pragma once

#include "GestureControllers/IGestureControllerSelector.h"
#include "HighlightFader.h"
#include "ILoopBoundariesController.h"
#include "IOrchestrionNotationInteractionProcessor.h"
#include "KineticScroller.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "OrchestrionSequencer/OrchestrionTypes.h"
#include "PositionEstimation/PositionEstimator.h"
#include "ReadingFocus.h"
#include "ScoreAnimation/ISegmentRegistry.h"
#include "ScoreFollower.h"
#include "TempoVizModel.h"
#include "TimingFeedbackOverlay.h"
#include <QElapsedTimer>
#include <QVariantList>
#include <actions/iactionsdispatcher.h>
#include <context/iglobalcontext.h>
#include <limits>
#include <notation/inotationconfiguration.h>
#include <notation/view/notationpaintview.h>
#include <unordered_map>
#include <vector>

namespace dgk
{
class OrchestrionNotationPaintView : public mu::notation::NotationPaintView,
                                     public ScoreFollower::Canvas
{
  Q_OBJECT
  // Debug tooltip shown when hovering a note (see noteInfoTooltipEnabled).
  // Empty when nothing relevant is hovered, which hides the tooltip.
  Q_PROPERTY(QString hoveredNoteInfo READ hoveredNoteInfo NOTIFY
                 hoveredNoteInfoChanged)
  Q_PROPERTY(QPointF hoveredNoteInfoPos READ hoveredNoteInfoPos NOTIFY
                 hoveredNoteInfoChanged)
  // How the tooltip sits relative to hoveredNoteInfoPos: 0 = below-right of
  // the cursor; 1 / 2 = vertically centred to the left / right of a timing
  // gauge's anchor (beside the onset's noteheads, clear of the coloured
  // copy — left for an early note, right for a late one).
  Q_PROPERTY(int hoveredNoteInfoPlacement READ hoveredNoteInfoPlacement NOTIFY
                 hoveredNoteInfoChanged)
  // Whether the last right-click hit a chord — enables the "set loop
  // start/end" context-menu items.
  Q_PROPERTY(bool contextMenuHasTarget READ contextMenuHasTarget NOTIFY
                 contextMenuTargetChanged)
  // Real-time tempo-model visualization (shown beneath the score, toggled from
  // the Advanced menu). The model is fed by the follower; the flag mirrors the
  // persisted config setting.
  Q_PROPERTY(dgk::TempoVizModel *tempoVizModel READ tempoVizModel CONSTANT)
  Q_PROPERTY(bool tempoVisualizationEnabled READ tempoVisualizationEnabled
                 NOTIFY tempoVisualizationEnabledChanged)
  // The take's final score (0–100) and its component breakdown (e.g.
  // "tempo 87 · sync 92"), set when the piece's last notes are released;
  // −1 = no banner. QML shows them as the end-of-piece banner and dismisses
  // via dismissFinalScore().
  Q_PROPERTY(int finalScore READ finalScore NOTIFY finalScoreChanged)
  Q_PROPERTY(QString finalScoreBreakdown READ finalScoreBreakdown NOTIFY
                 finalScoreChanged)
  //! The reasoning behind the banner's score, for its expandable panel: one
  //! entry per component, each {label, score, detail}.
  Q_PROPERTY(QVariantList finalScoreMetrics READ finalScoreMetrics NOTIFY
                 finalScoreChanged)
  // Post-take tuning: the tempo model's smoothing memory γ, exposed as a
  // slider once the take is over — writing it re-fits the whole take (curve,
  // tooltips, stats, layout warp) so the effect is observable immediately.
  Q_PROPERTY(bool smoothingTunerVisible READ smoothingTunerVisible NOTIFY
                 smoothingTunerVisibleChanged)
  Q_PROPERTY(double tempoSmoothing READ tempoSmoothing WRITE setTempoSmoothing
                 NOTIFY tempoSmoothingChanged)

  muse::Inject<IOrchestrionNotationInteractionProcessor> interactionProcessor;
  muse::Inject<ILoopBoundariesController> loopBoundariesController;
  muse::Inject<mu::notation::INotationConfiguration> configuration;
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<IOrchestrion> orchestrion;
  muse::Inject<IGestureControllerSelector> gestureControllerSelector;
  muse::Inject<ISegmentRegistry> chordRegistry;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;
  muse::Inject<IOrchestrionSequencerConfiguration> sequencerConfiguration;

public:
  explicit OrchestrionNotationPaintView(QQuickItem *parent = nullptr);

  Q_INVOKABLE void loadOrchestrionNotation();

  QString hoveredNoteInfo() const;
  QPointF hoveredNoteInfoPos() const;
  int hoveredNoteInfoPlacement() const { return m_hoveredNoteInfoPlacement; }
  TempoVizModel *tempoVizModel() { return &m_tempoVizModel; }
  bool tempoVisualizationEnabled() const;
  int finalScore() const { return m_finalScore; }
  QString finalScoreBreakdown() const { return m_finalScoreBreakdown; }
  QVariantList finalScoreMetrics() const { return m_finalScoreMetrics; }
  Q_INVOKABLE void dismissFinalScore();
  bool smoothingTunerVisible() const;
  double tempoSmoothing() const;
  void setTempoSmoothing(double memory);

  bool contextMenuHasTarget() const;
  Q_INVOKABLE void contextMenuSetLoopStart();
  Q_INVOKABLE void contextMenuSetLoopEnd();
  Q_INVOKABLE void clearLoop();

signals:
  void mouseActivity();
  void hoveredNoteInfoChanged();
  void contextMenuTargetChanged();
  //! Right-click: ask QML to pop up the loop context menu at \p position
  //! (view-local coordinates).
  void contextMenuRequested(QPointF position);
  void tempoVisualizationEnabledChanged();
  void finalScoreChanged();
  void smoothingTunerVisibleChanged();
  void tempoSmoothingChanged();

private:
  void onLoadNotation(mu::notation::INotationPtr notation) override;
  void onMatrixChanged(const muse::draw::Transform &oldMatrix,
                       const muse::draw::Transform &newMatrix,
                       bool overrideZoomType = true) override;
  void subscribe(const IOrchestrionSequencer &sequencer,
                 const IModifiableItemRegistry &registry);
  void constrainScorePosition();
  //! (Re)connect the follow to \p window's per-frame hook.
  void connectFrameTick(QQuickWindow *window);
  //! Clamp a desired viewport-left (logical) so the empty space past either end
  //! of the system never exceeds the max padding (and a system narrower than
  //! the view stays centered). Shared by manual constraint and auto-follow.
  double clampLeftX(double desiredLeftX, double scaling) const;
  //! The next barline past which the reading does not carry on from unrolled
  //! (playback) tick \p utick, and where it resumes — see
  //! ScoreFollower::Barrier. Found from the score's expanded repeat list,
  //! whose unrolled ticks are the sequencer's: the reading is linear through
  //! consecutive unrolled segments as long as each starts at the score tick
  //! where the previous ends; the first that doesn't (a jump back at a
  //! repeat's end, a volta skipped, a D.S./coda) or the last one is left at
  //! the end of its last measure, and the reading resumes at the start of the
  //! segment after it. With it, the unrolled tick of the first note after the
  //! jump — the first chord (any track) in the segment resumed at, or the
  //! jump itself — for the anticipation (see resumeExpectedInMs()).
  struct BarrierAhead
  {
    ScoreFollower::Barrier barrier;
    std::optional<int> resumeUtick;
  };
  std::optional<BarrierAhead> nextBarrier(int utick) const;
  void setViewMode(mu::notation::ViewMode);
  bool eventFilter(QObject *watched, QEvent *event) override;
  void paint(QPainter *painter) override;
  void paintNotationUnderlay(QPainter *painter) override;
  //! Orchestrion-styled loop boundaries (replaces MuseScore's orange flags):
  //! navy pill handles with a cream accent, plus — via the underlay — a soft
  //! cream tint across the looped span.
  void paintLoopMarkers(muse::draw::Painter *painter) override;
  void paintLoopRegionUnderlay(QPainter *painter);
  //! Post-take: a vertical grid line behind the score at each beat of the
  //! fitted tempo curve. With the performance warp baked (x = performed
  //! time), the lines' spacing is the performed beat duration: they spread
  //! where the performer slowed and bunch where they rushed.
  void paintBeatLines(QPainter *painter);
  void onMousePressed(const QPointF &pos, Qt::KeyboardModifiers modifiers,
                      Qt::MouseButton button);
  void onMouseDragged(const QPointF &pos, Qt::MouseButtons buttons);
  void onMouseReleased(Qt::MouseButton button);
  void onMouseMoved(const QPointF &pos);
  //! The loop-boundary flag at \p logicPos (canvas coordinates), if any.
  std::optional<mu::notation::LoopBoundaryType>
  loopFlagAt(const muse::PointF &logicPos) const;
  //! Move the dragged loop boundary to the chord under \p logicPos.
  void dragLoopBoundaryTo(const muse::PointF &logicPos);
  void updateHoveredNoteInfo(const QPointF &itemPos);
  void setHoveredNoteInfo(const QString &info, const QPointF &itemPos);
  std::vector<mu::engraving::EngravingItem *>
  getRelevantItems(TrackIndex track,
                   const mu::engraving::Segment *segment) const;
  void OnTransitions(const std::map<TrackIndex, ChordTransition> &transitions);
  //! Per rendered frame: advance the scroll and the estimate, sample the
  //! debug tempo strip, and fire the auto-played hand's due events.
  void onFrameTick();
  //! Refresh the tempo-following auto-play targets (the auto hand's next due
  //! release/strike, in playback-unrolled ticks) from a transitions batch.
  //! Batches only carry the *changed* tracks, so a per-voice ledger
  //! (m_autoTrackTargets) persists the auto staff's state between batches.
  void updateAutoTargets(const std::map<TrackIndex, ChordTransition> &batch);
  //! Which hand the machine plays, or −1: the setting, gated on auto-play
  //! being exposed at all.
  int autoPlayedStaff() const;
  //! Fire the auto hand's due strike or release once the manual hands'
  //! estimate has reached it.
  void fireDueAutoEvents(double nowMs);
  //! Measure this batch's played velocities against each hand's smoothed
  //! loudness curve — the dynamics counterpart of the timing judgments.
  //! \p resumingHands are the hands whose estimate just restarted after a
  //! stop, so their swell restarts too.
  std::map<int /*staff*/, std::vector<PositionEstimator::Judgment>>
  judgeDynamics(double nowMs, const std::map<int /*staff*/, double> &velocities,
                const std::vector<int> &resumingHands);
  //! Bake the take's fitted tempo curve into the score layout, so the page
  //! shows performed time and the coloured performance notes sit at
  //! residual-only offsets. Fires once per take, when it is over (end of
  //! piece or an interruption); self-guards on the proportional-spacing
  //! mode. Animated by default; instant when re-tuning.
  void bakePerformanceWarp(bool animate = true);
  void applyWarpStep();
  //! The take is over (interruption or end of piece): hand its recording to
  //! the automatic player — the play button now replays the performance —
  //! and bake its tempo curve into the layout.
  void endTake();
  //! Arm (or disarm) the play button with the finished take, per the current
  //! play mode: the raw performance, its fitted-tempo idealization, or
  //! nothing (metronomic playback).
  void pushReplayTake();
  //! The take's events time-warped onto the fitted tempo curve: each event
  //! shifted by the (interpolated) fitted error at its time, so what plays
  //! is the spline — the performance minus its per-note jitter.
  std::vector<ReplayEvent> fittedTempoEvents() const;
  //! Re-fit the whole take with the configured smoothing memory and refresh
  //! everything derived from it (ribbon, tooltips, stats, layout warp).
  void retuneTake();
  //! The re-fit itself: judge every take onset against one full-hindsight
  //! (unbounded-window) spline and push the verdicts through the
  //! display/stats pipeline. Runs once when the take ends — the live,
  //! bounded-window verdicts are provisional — and again per γ retune.
  void refitTakeJudgments();
  //! Back to the ideal (notated) spacing — a fresh take is starting.
  void clearPerformanceWarp();
  void updateNotation();

  // ScoreFollower::Canvas
  double viewWidth() const override { return width(); }
  double viewScaling() const override { return currentScaling(); }
  double anchorX() const override;
  void centerOn(double logicalX) override;
  //! From the live tempo and position of the hands being tracked (the
  //! earliest of them) to m_resumeUtick — or nothing while none is. The
  //! estimated position is never taken past the next note to play
  //! (m_focusUtick): between onsets the tracker extrapolates at the last
  //! tempo, through a hesitation as through a held note, and a jump must not
  //! be anticipated on notes the performer has not got to yet. So a freeze
  //! leaves the expected time at what separates the next note from the jump,
  //! while a note held before the jump lets it run down to the lead.
  std::optional<double> resumeExpectedInMs() const override;
  void wheelEvent(QWheelEvent *event) override;
  //! Zoom the score in/out about the cursor in response to a Ctrl-modified
  //! wheel event (mouse wheel or two-finger trackpad swipe).
  void zoomBy(const QWheelEvent &event);
  //! Pan the canvas by \p physicalDx physical pixels; returns whether it moved
  //! (false ⇒ clamped at an edge). Drives the KineticScroller.
  bool moveCanvasBy(qreal physicalDx);
  void initTouchpadMidiController();
  float hitWidth() const;

  // Live highlight per track (ringing or upcoming note).
  std::unordered_map<int, Highlight> m_boxes;
  // Highlights of just-ended notes, fading out (owns its own timer/clock).
  HighlightFader m_fader;
  // Timing-judgment feedback: per-onset error gauges next to the notes plus
  // the recent-error box plot, drawn on top of the notation.
  TimingFeedbackOverlay m_timingOverlay;
  // Set on any interruption of play (stop/jump, click, swipe, manual zoom):
  // the stats stay readable, but start over when playing resumes.
  bool m_timingStatsStale = false;
  // The latest gesture's raw controller velocity per hand (empty for
  // velocity-less devices), from HandNoteEvents — which fires just before the
  // transitions batch the gesture causes; consumed by that batch's onsets.
  std::optional<float> m_pendingHandVelocity[2] = {}; // [0]=right, [1]=left
  // Per-voice ledger backing updateAutoTargets(), and the auto hand's next
  // due release/strike aggregated from it.
  struct AutoTargets
  {
    std::optional<double> offTick;
    std::optional<double> onTick;
  };
  std::map<int /*track value*/, AutoTargets> m_autoTrackTargets;
  std::optional<double> m_autoOffTick;
  std::optional<double> m_autoOnTick;
  // Polls those targets against the manual hands' estimate. A driver of its
  // own, not the frame hook: between page turns nothing is animating, so no
  // frames are rendered — and the auto hand must play on regardless.
  QTimer m_autoPlayTimer;

  // The take's onsets, for baking the performance's tempo warp into the
  // layout: identity (staff, tMs) to look up the final revised error in the
  // overlay, plus the onset's engraved (score) tick and playback tick.
  struct TakeOnsetRecord
  {
    int staff;
    double tMs;
    int scoreTick;
    double utick;
    // The same instant on the replay recording's clock, linking the onset's
    // fitted error to the recorded events (for the fitted-tempo replay).
    double eventMs;
    // The onset's engraved segment element, for its live x (follows the
    // warp morph) — anchors the post-take beat grid.
    const mu::engraving::EngravingItem *anchor = nullptr;
  };
  std::vector<TakeOnsetRecord> m_takeOnsetRecords;
  // The take's raw input events (times relative to its first event) for the
  // post-take replay, and the earliest score tick it struck — where the
  // replay rewinds to.
  std::vector<ReplayEvent> m_replayEvents;
  QElapsedTimer m_replayClock;
  int m_replayStartTick = std::numeric_limits<int>::max();
  // Whether the recorded take is finished (armed for replay): a play-mode
  // change may then re-arm it, but never a half-recorded one.
  bool m_takeOver = false;
  // The baked warp (score tick → warped ticks) and its ease-in animation.
  std::vector<std::pair<int, double>> m_warpTable;
  QTimer m_warpTimer;
  double m_warpProgress = 0.0;
  bool m_warpBaked = false;
  // The final-score banner fires once per take, when the piece's last notes
  // are released; re-armed when the stats restart. −1 = no banner showing.
  bool m_finalScoreShown = false;
  int m_finalScore = -1;
  QString m_finalScoreBreakdown;
  QVariantList m_finalScoreMetrics;

  struct Contact
  {
    const bool isLeft;
    double x = 0.;
    double y = 0.;
  };

  std::unordered_map<int, Contact> m_contacts;
  bool m_constrainingScorePosition = false;
  QPoint m_lastCursorPos{-1, -1};

  // Rolling state of the tempo model for the visualization strip; fed by the
  // follower. Declared before m_follower so it exists when the follower (which
  // writes to it) is constructed.
  TempoVizModel m_tempoVizModel;

  // Where each hand has got to, and the event the page keeps in view — the
  // leading hand's reading (see ReadingFocus). Cleared when the position
  // jumps; the jump's transitions batch repopulates it.
  ReadingFocus m_readingFocus;
  // Turns the played events into a page-turning scroll at the user's zoom.
  // While it drives the canvas (centerOn), m_drivingScroll makes
  // constrainScorePosition() yield so it isn't undone.
  ScoreFollower m_follower;
  // Where each hand has got to, and how far each onset fell from the
  // performer's own smooth curve: the grading's raw material. Its clock is
  // this view's — the timestamps it hands back identify the onsets.
  PositionEstimator m_estimator;
  QElapsedTimer m_clock;
  //! The unrolled tick of the focus — the next note to play — and of the
  //! first note after the jump ahead of it, when there is one (see
  //! nextBarrier / resumeExpectedInMs).
  std::optional<int> m_focusUtick;
  std::optional<int> m_resumeUtick;
  // The loudness curves the dynamics judgments are measured against: one per
  // hand, fed the controller velocities. Not the estimator's business —
  // loudness is not position — but the same smoother serves.
  std::map<int /*staff*/, PositionSmoother> m_loudness;
  bool m_drivingScroll = false;
  QMetaObject::Connection m_frameTickConnection;

  // Background left-drag pans the canvas (done by the base view); we sample the
  // drag so releasing it adds a kinetic throw via m_kineticScroller.
  bool m_canvasDragging = false;
  QPointF m_lastDragPos;

  // Kinetic ("flick") horizontal scrolling: a trackpad swipe can be "thrown"
  // and the viewport keeps gliding until it slows to a stop or hits the edge.
  KineticScroller m_kineticScroller;
  QString m_hoveredNoteInfo;
  QPointF m_hoveredNoteInfoPos;
  int m_hoveredNoteInfoPlacement = 0;

  // Chord under the last right-click, acted on by the context-menu items.
  std::optional<ILoopBoundariesController::ChordTicks> m_contextMenuTarget;

  // The loop-boundary flag being dragged, if any. While set, mouse events are
  // filtered away from the base view so it doesn't pan or select alongside.
  std::optional<mu::notation::LoopBoundaryType> m_draggedLoopBoundary;
  // A horizontal-resize override cursor is active (hovering a loop flag).
  bool m_loopFlagCursor = false;
};
} // namespace dgk