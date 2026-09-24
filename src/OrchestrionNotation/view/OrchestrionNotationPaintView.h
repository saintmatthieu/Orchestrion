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

#include "HighlightFader.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
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
#include <QImage>
#include <QVariantList>
#include <actions/iactionsdispatcher.h>
#include <context/iglobalcontext.h>
#include <limits>
#include <notation/inotationconfiguration.h>
#include <notation/inotationcontextconfiguration.h>
#include <notationscene/qml/MuseScore/NotationScene/notationpaintview.h>
#include <ui/iuiconfiguration.h>
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
  // The score reads as the belt of a treadmill: at either end it wraps a
  // quarter turn around a cylinder and runs away from the viewer. The
  // cylinder's radius is this fraction of the parchment's height, so that the
  // two keep their proportions when the score is zoomed — a rod does not get
  // thinner because the belt on it got wider. 0 = flat ends.
  Q_PROPERTY(double rollRadiusRatio READ rollRadiusRatio WRITE
                 setRollRadiusRatio NOTIFY rollRadiusRatioChanged)
  // How far short of the view's edge the belt goes edge-on, in pixels: the
  // strip outside it is backdrop and nothing else.
  Q_PROPERTY(double rollMargin READ rollMargin WRITE setRollMargin NOTIFY
                 rollMarginChanged)
  // How steeply the belt falls into shadow as it turns: it is darkened by
  // cos(turn) raised to this power, so 1 is the plain Lambert law and higher
  // values keep it lit until the last moment.
  Q_PROPERTY(double rollShadePower READ rollShadePower WRITE setRollShadePower
                 NOTIFY rollShadePowerChanged)
  // How much clear parchment there is around the notation, in pixels. The
  // parchment is sized to the score's skyline, so this is margin on top of
  // everything the engraving actually puts there — at its ends as well as
  // above and below.
  Q_PROPERTY(double beltPadding READ beltPadding WRITE setBeltPadding NOTIFY
                 beltPaddingChanged)
  // How many pixels of the cylinders show past the parchment they carry, top
  // and bottom. 0 hides them.
  Q_PROPERTY(double rollerOverhang READ rollerOverhang WRITE setRollerOverhang
                 NOTIFY rollerOverhangChanged)
  // How far down the view the title's ornament reaches, from QML: zooming in
  // stops before the treadmill would come within a margin of it.
  Q_PROPERTY(double titleClearance READ titleClearance WRITE setTitleClearance
                 NOTIFY titleClearanceChanged)

  dgk::Inject<IOrchestrionNotationInteractionProcessor> interactionProcessor{this};
  dgk::Inject<ILoopBoundariesController> loopBoundariesController{this};
  dgk::Inject<mu::notation::INotationConfiguration> configuration{this};
  dgk::Inject<mu::notation::INotationContextConfiguration> contextConfiguration{this};
  dgk::Inject<mu::context::IGlobalContext> globalContext{this};
  dgk::Inject<IOrchestrion> orchestrion{this};
  //! For the Orchestrion palette (see OrchestrionPalette.h): what this view
  //! paints itself — highlights, loop markers, the beat grid — follows the
  //! theme the View menu chose.
  dgk::Inject<muse::ui::IUiConfiguration> uiConfiguration{this};
  dgk::Inject<ISegmentRegistry> chordRegistry{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<IOrchestrionSequencerConfiguration> sequencerConfiguration{this};

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
  double rollRadiusRatio() const { return m_rollRadiusRatio; }
  void setRollRadiusRatio(double ratio);
  double titleClearance() const { return m_titleClearance; }
  void setTitleClearance(double clearance);
  double rollMargin() const { return m_rollMargin; }
  void setRollMargin(double margin);
  double rollShadePower() const { return m_rollShadePower; }
  void setRollShadePower(double power);
  double beltPadding() const { return m_beltPadding; }
  void setBeltPadding(double padding);
  double rollerOverhang() const { return m_rollerOverhang; }
  void setRollerOverhang(double overhang);

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
  void rollRadiusRatioChanged();
  void titleClearanceChanged();
  void rollMarginChanged();
  void rollShadePowerChanged();
  void beltPaddingChanged();
  void rollerOverhangChanged();

private:
  void onLoadNotation(mu::notation::INotationPtr notation) override;
  void onUnloadNotation(mu::notation::INotationPtr notation) override;
  void onMatrixChanged(const muse::draw::Transform &oldMatrix,
                       const muse::draw::Transform &newMatrix,
                       bool overrideZoomType = true) override;
  void onViewSizeChanged() override;
  void subscribe(const IOrchestrionSequencer &sequencer,
                 const IModifiableItemRegistry &registry);
  void constrainScorePosition();
  /**
   * The logical y that puts the parchment in the middle of the view — the
   * page's own margins are not symmetric about the music, so centring those
   * leaves the belt and its cylinders sitting off centre. Shared by the
   * constraint and by the follow, which both place the canvas: if they
   * disagree, they pull it up and down between them.
   */
  double centredTopY(double scaling) const;
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
  //! slate pill handles with a pearl accent, plus — via the underlay — a
  //! soft slate tint across the looped span.
  void paintLoopMarkers(muse::draw::Painter *painter) override;
  void paintLoopRegionUnderlay(QPainter *painter);
  //! Loop-marker palette: the wallpaper's own backdrop colour for the
  //! handles and the region shading, the accent for the dot on the tab.
  QColor loopHandleColor() const;
  QColor loopAccentColor() const;
  //! Post-take: a vertical grid line behind the score at each beat of the
  //! fitted tempo curve. With the performance warp baked (x = performed
  //! time), the lines' spacing is the performed beat duration: they spread
  //! where the performer slowed and bunch where they rushed.
  void paintBeatLines(QPainter *painter);
  //! Everything this view draws — background, highlights, notation, loop
  //! marks, timing gauges — in one call, so that it can be rendered either
  //! straight onto the item or into the roll buffer.
  void paintContents(QPainter *painter);
  /**
   * Zooming is bounded by the machine the score sits on: wound right out, the
   * parchment has to fit between the two cylinders — nothing left on either
   * roll — and wound in, the belt must not climb into the title. Both ends
   * are closed forms, since the cylinders' size follows the parchment's.
   */
  void setScaling(qreal scaling, const muse::PointF &pos,
                  bool overrideZoomType = true) override;
  std::pair<double, double> scalingLimits() const;

  //! The margin and core radius the rolls are drawn with, clamped to what the
  //! view can hold. Shared by the painting and by the scroll limits, which
  //! stop when the parchment's end reaches a cylinder's crown.
  std::pair<double, double> rollBase() const;
  /**
   * The whole treadmill's geometry, sampled once and then passed around.
   * Sampled, because paint() runs on the render thread while the main thread
   * may be scrolling the canvas: anything that reads the matrix again part
   * way through a frame draws its share of the picture at a slightly later
   * scroll position than the rest, and the parchment, the rolls, the
   * cylinders and the tacks drift apart by a pixel or two — a jitter that
   * shows only while the score is moving, and never once it is still.
   */
  struct BeltGeometry
  {
    bool valid = false;
    QRectF paper;        //!< the parchment, in logical (score) coordinates
    double originX = 0;  //!< where logical 0 falls on the view's axes...
    double originY = 0;
    double scaling = 1;  //!< ...and the scale that goes with it
    double viewWidth = 0;
    double viewHeight = 0;
    //! The rows the parchment occupies, clamped to the view. Only these have
    //! to be rendered into the roll buffers and warped: everywhere else the
    //! wallpaper is the same all the way across, so wrapping it changes
    //! nothing.
    double bandTop = 0;
    double bandBottom = 0;
    double margin = 0;
    double core = 0;        //!< the bare cylinder
    double radius[2] = {};  //!< what each end actually wraps around: [left, right]

    double viewX(double logicalX) const { return originX + logicalX * scaling; }
    double viewY(double logicalY) const { return originY + logicalY * scaling; }
    //! How far the parchment has run, in view pixels: what turns the rolls.
    double travel() const { return -originX; }
    double wrap(bool leftEnd) const { return radius[leftEnd ? 0 : 1]; }
    //! Where the parchment leaves the flat run and starts round the cylinder.
    double tangent(bool leftEnd) const
    {
      return leftEnd ? margin + wrap(true) : viewWidth - margin - wrap(false);
    }
    //! The view x of the parchment's near end at this cylinder, before the
    //! wrap is taken into account.
    double flatEnd(bool leftEnd) const
    {
      return viewX(leftEnd ? paper.left() : paper.right());
    }
  };
  BeltGeometry sampleBelt() const;

  /**
   * One end of the belt. The flat run of the score spans [radius, width() -
   * radius]; past its tangent point the belt turns a quarter circle away from
   * the viewer, so the last `radius` pixels of the view show
   * `radius * pi / 2` pixels of score — including what lies just beyond the
   * view — squeezed as the sine of the turn and shaded as its cosine.
   */
  void paintRoll(QPainter *painter, const BeltGeometry &belt, bool leftEnd);
  /**
   * The stretch of score one roll shows, rendered flat at device resolution
   * with item x = \p originX at the image's left edge. \p clipItem, when
   * given, restricts the painting to that slice of it — everything outside is
   * left as it was, which is what makes the strip cache worth having.
   */
  void renderStrip(QImage &image, int originX, int originY, int logicalWidth,
                   int logicalHeight, double dpr,
                   const QRectF *clipItem = nullptr);
  /**
   * One end's flat strip, kept between frames. While the score is scrolling,
   * this frame's strip is the last one shifted along, so only the sliver of
   * belt that has just come into it has to be painted — the rest is a row-wise
   * move. Everything in it travels with the parchment (its own fill is
   * uniform, its grain is anchored in score space, the ink and the highlights
   * move with the notes), which is what makes the shift legitimate; the
   * wallpaper, which is pinned to the viewport and would smear, is outside the
   * band and no longer in here.
   */
  struct StripCache
  {
    QImage image;
    bool valid = false;
    int originX = 0, originY = 0, logicalWidth = 0, logicalHeight = 0;
    double dpr = 0, scaling = 0;
    //! The belt travel the pixels correspond to, which lags the current one
    //! by up to half a device pixel — the blit carries that remainder.
    double travel = 0;
    //! Frames since it was painted in full; a full repaint every so often is
    //! the backstop for anything that changed in place without scrolling.
    int age = 0;
  };
  //! Prepare one end's strip for this frame, in full or by shifting.
  StripCache &updateStrip(bool leftEnd, const BeltGeometry &belt, int originX,
                          int originY, int logicalWidth, int logicalHeight,
                          double dpr);
  //! (Re)build the backdrop strips that fill the margins, where the belt has
  //! already gone. A no-op unless the view size, the backdrop or the margin
  //! changed.
  void ensureMarginBackdrops(double margin);
  /**
   * The parchment, in logical (score) coordinates: everything the engraving
   * lays out — the staves and whatever the skyline says sticks out of them —
   * grown by beltPadding on every side. Nothing when no score is laid out.
   */
  std::optional<QRectF> beltRect() const;
  //! What the parchment is sized to before its padding: everything the
  //! engraving lays out, staves and skyline, in logical coordinates.
  std::optional<QRectF> scoreExtent() const;
  //! The parchment itself, drawn behind the notation (from the underlay hook)
  //! so that the score reads as printed on something.
  void paintBelt(QPainter *painter);
  /**
   * The cylinders the parchment is wound on, at either end: as wide as their
   * diameter, a little taller than the parchment, lit as a barrel. Drawn last
   * and clipped to where they are not covered — the overhang above and below
   * the parchment, and whatever crown the parchment has not yet come round.
   */
  void paintRollers(QPainter *painter, const BeltGeometry &belt);
  //! Where the parchment's near end lies on the view's x axis, following it
  //! around the cylinder once it is past the tangent point.
  double paperEdgeX(const BeltGeometry &belt, bool leftEnd) const;
  /**
   * A little tile of grain, laid over the parchment and the cylinders so that
   * neither is a perfectly flat fill. Built once, from a fixed seed, so the
   * same flecks land in the same places every run.
   */
  const QPixmap &grainTile();
  //! How much light the cylinder's surface catches at \p across (−1 to 1 over
  //! the barrel, 0 at the crown).
  static double barrelLight(double across);
  //! Turns of parchment already wound past this end's tangent point: none at
  //! the end the score is wound back to, the whole roll at the other.
  double woundTurns(const BeltGeometry &belt, bool leftEnd) const;
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

  // The treadmill ends: the cylinder radius and the scratch buffer the belt
  // is rendered flat into before being wrapped around it (device pixels,
  // reused between frames).
  double m_rollRadiusRatio = 0.0;
  double m_titleClearance = 0.0;
  double m_rollMargin = 0.0;
  double m_rollShadePower = 1.0;
  double m_beltPadding = 0.0;
  double m_rollerOverhang = 0.0;
  //! The parchment's centre the last time painting asked for the canvas to be
  //! re-centred, so that a position the constraint cannot reach is asked for
  //! once and not every frame.
  double m_lastRecentreAt = 0.0;
  QPixmap m_grain;
  StripCache m_strip[2]; //!< [left, right]
  //! Something other than the scroll changed what the strips show: paint them
  //! in full next time.
  bool m_stripsDirty = true;
  //! The theme the strips were painted in. Watched rather than subscribed to:
  //! the base view already holds the one currentThemeChanged() subscription
  //! this object may have, and a second would replace it.
  muse::ui::ThemeCode m_stripThemeKey;
  QPixmap m_marginBackdropLeft;
  QPixmap m_marginBackdropRight;
  QSize m_marginBackdropViewSize;
  qint64 m_marginBackdropSourceKey = 0;
  int m_marginBackdropWidth = -1;
};
} // namespace dgk