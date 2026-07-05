# Tempo tracking — the maths

This document records the mathematics behind `TempoTracker` and why the design
converged on a **Kalman filter** (in its steady-state **α–β** form). It is meant
to be reviewed independently of the code, but it matches the current
implementation in `TempoTracker.{h,cpp}` and `view/TempoFollower.{h,cpp}`.

Notation: real (wall-clock) time `t` in ms; musical position `s` in some linear
musical coordinate (page-logical x for scrolling, or score ticks for tempo);
tempo / velocity `v = ds/dt`. Onsets (played notes) arrive at times `t_k` with
known musical positions `z_k`. Between onsets we get nothing.

---

## 1. The problem

From **sparse** onsets `(t_k, z_k)` we must produce, **in real time and between
onsets**, a smooth estimate of the current musical position `s(t)` and tempo
`v(t)`, so that:

- a **constant** performed tempo yields a **constant** estimated tempo (and a
  constant-speed scroll) - not necessarily, since the x spacing of rhythm symbols is determined by layout rules more than timing.

  > **A.** Right, and the distinction matters: this bullet is about the **tempo
  > estimate** (which _is_ constant for constant timing — proven in §5). The
  > **x-scroll speed** is deliberately _not_ constant, exactly for the reason you
  > give — layout x isn't proportional to time, so the view must vary its x-speed
  > to land on the unevenly-x-spaced notes on time (see §7). Two different
  > outputs of the same model.

- the estimate is **smooth** (no per-note steps/jumps) yet **responsive** to
  genuine tempo change;
- the a posteriori position estimate is a curve that passes through all the performance points. When a new point arrives, the position estimate is re-fitted and the estimate error slowly compensated for by re-centering of the score

  > **A.** One nuance: it's a _smoothing_ fit, not an _interpolating_ one, so the
  > curve does **not** pass exactly through every onset — the gain `α < 1` means
  > each onset only pulls the estimate part-way (§4). Passing exactly through all
  > points is the `λ → 0` limit you note in §2(d); we avoid it because it would
  > also fit the timing jitter (the "dents"). The "re-fitted on each new point,
  > error slowly compensated by re-centering" is spot on — that re-centering is
  > the **continuity offset** in §6.2.

- it is **causal** and **short-memory** (a few notes), not a batch fit;
- it has **few tuning knobs**;
- it is **model-agnostic** and reusable (scroll following now; rhythm grading
  later — the per-onset residual is the timing error).

---

## 2. Why a Kalman filter — the chain of reasoning

**(a) Tempo is the derivative of a position-vs-time curve.** Define the
_performance map_ `s(t)`. Then tempo is `v = ds/dt`. Estimating tempo = fitting a
smooth `s(t)` to the onsets and differentiating it.

**(b) A ritardando is locally a straight tempo line** — i.e. constant tempo
_change_, so `s(t)` is locally **parabolic**. But a performer cannot accelerate
or decelerate forever: tempo is piecewise-linear _at best_, with **breaks**
between a steady section and a ritardando. Fitting one global low-order
polynomial across such a break **under-fits** at the break and dumps the misfit
into the residual — it would flag a clean ritardando as "error". The natural
remedy for "piecewise-polynomial with breaks" is a **spline** (explanation pls).

> **A. What is a spline?** A piecewise polynomial: cut the domain at points
> called _knots_, fit a low-degree polynomial on each piece, and join the pieces
> with a prescribed smoothness at the knots (e.g. matching value and slope). A
> _smoothing_ spline doesn't pass through every data point — it minimizes
> `(fit error) + λ·(roughness)`, so the single knob `λ` trades fidelity against
> smoothness. "Piecewise-linear-tempo with breaks" is just a spline whose pieces
> are chosen so tempo is piecewise linear.

**(c) Which continuity?** The break between _steady_ and _ritardando_ is a jump (rather an impulse?)
in **acceleration** (`s''`), while position (`s`) and tempo (`s'`) stay

> **A. Jump vs impulse.** _Acceleration_ `s''` **jumps** (a step: from 0 to a
> constant value) — not an impulse. The **jerk** `s'''` (its derivative) is the
> **impulse** at that instant. Over a whole ritardando there are two: a `+`
> impulse in jerk where it begins (accel steps up) and a `−` impulse where it
> ends (accel steps back to 0) — exactly your "one direction and another in the
> opposite" in (e) below.
> continuous. So the right object is a spline that is **C¹ but not C²** (explanation pls) — it allows
> acceleration breaks for free (the intended ritardando onset) while keeping tempo
> continuous. Equivalently: penalize **jerk** (`s'''`) (derivative of an impulse? An impulse in the one direction and another in the opposite?), so that constant tempo
> _and_ constant ritardando are both free and only _changes of tempo-change_ cost.

> **A. Cⁿ continuity.** "Cⁿ" means the function and its first `n` derivatives are
> all continuous. C⁰ = position continuous (no teleport); **C¹** = position _and_
> tempo continuous (no instantaneous speed jump); C² = also acceleration
> continuous. We want **C¹ but not C²**: tempo stays continuous, but acceleration
> may jump — which is precisely a steady→ritardando corner.
>
> **A. Penalizing jerk.** Yes — and your "+ then −" intuition is exactly right
> (see the jump-vs-impulse note above). "Penalize jerk" means the prior favours
> _small_ `∫(s''')²`: zero jerk = constant acceleration = a straight tempo line,
> so **constant tempo** (accel 0) and **constant ritardando** (accel constant)
> both cost nothing; only the _corners_ (jerk impulses, i.e. tempo-change onsets)
> are charged. That's what makes a ritardando "free" but its start/stop "paid
> for". (An L²-on-jerk prior actually rounds true impulses slightly; the point is
> the _shape_ it prefers, not that impulses are literally free.)

**(d) Real-time forces the recursive form.** A smoothing spline is a _batch_ fit
over all data. The online equivalent comes from a classical duality:

> The smoothing spline that minimizes `∑ (z_k − s(t_k))² + λ ∫ (s⁽ⁿ⁾)² dt` (if we retrofit the line in the gesture events, then `∑ (z_k − s(t_k))² = 0`) is the
> posterior mean of a state-space model whose `n`-th derivative is white noise
> (an _n_-times integrated Wiener process). Running a **Kalman smoother/filter**
> on that model _is_ the spline; `λ` corresponds to the process/measurement noise
> ratio.

(That's the part I need to learn about. State-space model? Wiener process?)

> **A. State-space model.** Describe a system by a hidden **state** vector that
> evolves step-to-step by a (linear) rule plus random "process noise", and that
> you observe only indirectly through noisy measurements:
> `xₖ = F·xₖ₋₁ + w` (evolution), `zₖ = H·xₖ + r` (observation). Here the state is
> `(position, tempo)`. A **Kalman filter** is the optimal recursive estimator of
> that state for linear-Gaussian models: it alternates **predict** (push the
> state forward by `F`) and **update** (correct using the new measurement).
>
> **A. Wiener process.** The continuous-time integral of white noise — Brownian
> motion: a continuous random walk with independent zero-mean Gaussian
> increments. "`n`-times integrated Wiener process" = white noise drives the
> `n`-th derivative, and you integrate `n` times to get position. `n=2`
> (white-noise acceleration; tempo random-walks) ⇒ the cubic smoothing spline ⇒
> our 2-state model.
>
> **A. On your `∑(…)²=0` note.** That (curve through every onset) is the
> _interpolating_ limit `λ → 0`. We keep `λ > 0` (smoothing) on purpose, so the
> curve does **not** pass exactly through the onsets — otherwise it chases the
> ±5 % timing jitter. (Ties back to your §1 "passes through all points" bullet.)
>
> **Best single reference:** Simo Särkkä, _Bayesian Filtering and Smoothing_
> (free PDF) — covers state-space models, Wiener processes, the Kalman filter,
> and even the spline ↔ Kalman equivalence, in that order.

So "fit a smooth position curve, in real time" **is** a Kalman filter. That is
what brought us here.

**(e) Pragmatic order.** The fully-principled choice (penalize jerk ⇒ white noise
on acceleration ⇒ a 3-state model) (white noise on acceleration: the local impulses of a transition from steady to rit. aren't noise??) is more than the prototype needs. We use the
**constant-velocity** (2-state) model: position + tempo, with a random walk on
tempo (what does that mean?). It tracks ritardandi with a small lag rather than anticipating them — fine
for following, and it collapses to **one knob**. (Upgrading to the 3-state
"constant-acceleration" model is the natural next step if anticipation is wanted.)

> **A. Correction to the text above.** Penalizing **jerk** corresponds to white
> noise on the **jerk** (acceleration random-walks) ⇒ the **3-state** model.
> White noise on **acceleration** is the **2-state** model we actually use
> (≡ the cubic smoothing spline). I mislabeled it — read "(penalize jerk ⇒ white
> noise on the jerk ⇒ 3-state)".
>
> **A. "The impulses aren't noise??"** Correct — they aren't. "White noise" here
> is a **prior** (a modelling assumption), not a claim about the music. It says
> only: _we refuse to predict the highest derivative_ — we treat it as
> zero-mean and unpredictable. A real ritardando's jerk impulses are simply one
> outcome that prior permits; the filter then **infers** them from the onsets
> rather than presuming them. (Same idea as modelling a coin you can't predict as
> "50/50" — not a claim the coin is random, just that you won't bet on a side.)
>
> **A. "Random walk on tempo".** The model assumes tempo changes each step by an
> unpredictable, zero-mean amount: `vₖ = vₖ₋₁ + (small noise)`. So tempo has no
> fixed value and no built-in trend — it is free to drift, and the onsets pin it
> down. (A random walk on _acceleration_ instead would let tempo trend smoothly =
> the 3-state model.)

---

## 3. The state-space model

State, dynamics over an interval `Δt`, and measurement:

$$
x = \begin{bmatrix} s \\ v \end{bmatrix},\qquad
x_k = F\,x_{k-1} + w,\quad
F = \begin{bmatrix} 1 & \Delta t \\ 0 & 1 \end{bmatrix},\qquad
z_k = H\,x_k + r,\quad H = \begin{bmatrix} 1 & 0 \end{bmatrix}
$$

- `F` is constant-velocity prediction: `s ← s + v·Δt`, `v ← v`. (What's that left arrow saying?)

  > **A.** `←` is **assignment** ("becomes"), not equality. `s ← s + v·Δt` reads
  > "the new `s` is the old `s` plus `v·Δt`"; `v ← v` means "`v` is carried over
  > unchanged". It's the same `←` programmers write as `s = s + v*dt`. I use `←`
  > rather than `=` precisely so it isn't misread as the _equation_ `s = s + v·Δt`
  > (which would say `v·Δt = 0`). It's just `F` applied to the state, written out
  > row by row.

- `w ~ N(0, Q)` (what's `~` and `N(0, Q)`?) is **process noise** — a random walk on tempo (the prior that
  says "tempo drifts slowly"). Its intensity `q` (where is `q`?) is the spline's `λ`.

  > **A.** `~` reads "is distributed as" — `w ~ N(0, Q)` means "`w` is a random
  > draw from a **Normal (Gaussian) distribution** with mean `0` and covariance
  > `Q`". So `w` is zero-mean (no systematic push) random noise of size set by `Q`.
  > `q` is the scalar that sizes `Q`: for our 2-state random-walk-on-tempo model
  > `Q` is a fixed `2×2` matrix shape times `q`, so `q` is the single "how much may
  > tempo drift per unit time" number. (It's `q` rather than `Q` because once the
  > _shape_ is fixed, only its scalar magnitude is a free knob.) Larger `q` ⇒ the
  > filter trusts the model less and the onsets more ⇒ less smoothing — exactly the
  > role of small `λ` in the spline.

- The **measurement** is the onset's musical position `z_k`; `r ~ N(0, R)` is how
  far the performer's actual onset sits from the smooth model — i.e. their timing
  imprecision/expressiveness, variance `ρ`.

The standard Kalman recursion (predict then correct) would propagate the
covariance `P` each step and recompute the gain `K = P Hᵀ (HPHᵀ+R)⁻¹`. **Only the
ratio `q/ρ` matters** for the shape of the estimate (rescaling both leaves `K`
unchanged) — hence "one knob". (suggest online readings to understand this)

> **A. Why only the ratio.** The gain `K` is built from `P` (uncertainty about
> the state) and `R` (measurement noise). `P` itself grows with `q` and shrinks
> with the correction. If you scale _both_ `q` and `ρ` by the same factor `c`,
> every covariance in the recursion scales by `c` too, and in `K = PHᵀ(HPHᵀ+R)⁻¹`
> the `c` cancels top and bottom — `K` is unchanged. So absolute noise levels
> don't matter, only **how noisy the model is relative to the measurements**
> (`q/ρ`). That single ratio is what `γ` (and hence `α, β`) encodes — our "one
> knob".
>
> **Suggested reading** (shortest path first):
>
> - Wikipedia, **"Alpha beta filter"** — directly this filter; `α, β`, critical
>   damping, the steady-state-Kalman connection.
> - Greg Welch & Gary Bishop, **"An Introduction to the Kalman Filter"** (UNC
>   tech report / SIGGRAPH course) — the gentlest derivation of predict/correct,
>   `P`, and `K`.
> - Simo Särkkä, **"Bayesian Filtering and Smoothing"** (free PDF) — rigorous;
>   ch. 4 Kalman filter, and the chapter relating splines / Wiener processes to
>   state-space models (the `q ↔ λ` duality).

---

## 4. Steady state ⇒ the α–β filter (what we implement)

For a scalar constant-velocity model with stationary noise, the Kalman gain
**converges to a constant** `K = [α; β/Δt]`. Using that fixed gain _is_ the
**α–β filter** — so we skip covariance bookkeeping entirely and keep two fixed
gains. The recursion, exactly as in `addObservation`:

```
predict:     s⁻ = s + v·Δt                 # Δt = t_k − t_{k−1}
innovation:  y  = z_k − s⁻                  # the timing error
correct:     s  = s⁻ + α·y
             v  = v  + (β / Δt)·y           # β is per-interval ⇒ divide by Δt
```

`β/Δt` (rather than `β`) makes the velocity correction physically consistent
under **irregular** sampling (notes of different durations).

**One knob via fading memory.** We parameterize by a single memory factor
`γ ∈ (0,1)` and derive critically-damped gains (no overshoot):

$$
\alpha = 1 - \gamma^2, \qquad \beta = (1-\gamma)^2 .
$$

Higher `γ` = longer memory = smoother but laggier. Default `γ = 0.6`
(`α = 0.64`, `β = 0.16`).

**Seeding.** The α–β transient from `v = 0` would ramp up far too slowly, so on
the **second** onset we set `v = (z_2 − z_1)/Δt` directly, then run α–β from the
third onset.

**Between onsets.** `positionAt(t) = s + v·(t − t_last)` — a straight
constant-velocity extrapolation, evaluated every frame (~60 fps) for smooth
motion. `v` only changes at onsets.

---

## 5. Proof: exact timing ⇒ constant tempo

Let the input be perfectly periodic: `z_k = z_{k-1} + D` (constant musical step)
at `Δt = Δ` (constant). Claim: once `v = D/Δ`, the estimate is exactly constant.

Predict: `s⁻ = s + v·Δ`. If `s = z_{k-1}` then `s⁻ = z_{k-1} + (D/Δ)·Δ = z_k`, so
the innovation `y = z_k − s⁻ = 0`. Therefore `s = z_k` (unchanged form) and
`v += (β/Δ)·0 = D/Δ` (unchanged). The point `(s = z_k,\ v = D/Δ)` is a fixed
point with zero innovation — **constant forever**. The seed reaches it on onset
2, so the output is constant from then on.

This is exactly what the unit test asserts
(`TempoTrackerTests.ConstantTimingYieldsConstantTempo`: spread `< 10⁻⁶`).

**Corollary (the "dents").** Any wobble in the output therefore comes from wobble
in the **input** — `Δt` jitter or musical-spacing variation — not from the model.
For small interval jitter `δ`, one onset perturbs the velocity by

$$
\Delta v \approx -\,\beta\, v \,\frac{\delta}{\Delta},
$$

i.e. the α–β acts as a **low-pass**: it attenuates per-onset timing jitter by
≈ `β`. Measured ±5 % onset-delivery jitter becomes ≈ ±0.8–1 % tempo wobble. (This
is why the _raw_ single-interval ratio `Δz/Δt` looked "dented" and the **fitted**
α–β estimate does not — the running fit is the "retrofit".)

---

## 6. Two extensions the prototype needed

### 6.1 Coasting — don't run away when the performer stops

A filter with no input keeps predicting forward, so `s` grows without bound. We
add a deceleration when the next onset is **overdue**. With `Δ̄` an EMA of recent
inter-onset intervals (pause-length gaps excluded so a stop doesn't inflate it):

- _overdue_ when `t − t_last > coastTrigger · Δ̄`;
- while overdue, each frame: `v ← v · exp(−Δt / (coastFactor · Δ̄))`.

So tempo (hence the position's advance) decays exponentially to rest instead of
extrapolating off the end. `coastTrigger ≫ 1` (currently 15) so ordinary long
notes and ritardandi do **not** trip it — only a genuine stop does.

### 6.2 Continuity offset — no snap when a late correction lands

A large innovation (e.g. a long-overdue note) makes a step change in `s`. To keep
the _displayed_ position continuous, we absorb the step into a decaying offset.
On each onset:

$$
o_0 = s_{\text{displayed,before}} - s_{\text{after}},\qquad
o(t) = o_0 \, e^{-(t - t_0)/\tau_{\text{off}}},\qquad
\text{position}(t) = \underbrace{s + v(t-t_{\text{last}})}_{\text{baseAt}} + o(t).
$$

The output is exactly continuous at the onset (`o(t_0)=o_0` cancels the step),
then slides to the true fit over `τ_off` (currently 1000 ms).

---

## 7. Two coordinates: scrolling (x) vs tempo readout (ticks)

The model is coordinate-agnostic; we run it in **two** coordinates per hand
(staff), because they answer different questions.

- **Scroll** is tracked in **page-logical x** (`onsetX`): `positionAt → x`, kept
  at the playhead. This is correct for scrolling: layout x is _not_ proportional
  to musical time (MuseScore's note spacing is sub-linear in duration), so the
  view _must_ vary its x-speed to land on unevenly-x-spaced notes at even times.
- **Tempo** is therefore **not** `dx/dt` — that wobbles with note density even at
  a constant tempo (confirmed in logs: constant `Δtick`, varying `Δx`). For the
  readout we run a **second** α–β tracker per hand fed **score ticks**; its
  `speed()` is genuine musical tempo, converted to BPM:

$$
\text{BPM} = v_{\text{tick}} \cdot \frac{60000}{\text{ticksPerQuarter}},\qquad
\text{ticksPerQuarter} = 480 .
$$

Repeats are handled by feeding each tracker a monotonic coordinate: the scroll
tracker folds each backward jump into an accumulating x-offset (kept monotonic,
subtracted for display); the tempo tracker is `reset()` at the backward tick.

---

## 7b. Getting the actual spline: fixed-lag smoothing (`TempoSmoother`)

The α–β filter above is *causal*: its tempo output is a **staircase** (constant
between onsets, nudged at each one), because the spline ↔ Kalman duality of
§2(d) holds for the *smoother* — the estimate given past **and future** onsets —
while the filter only ever exposes the spline's right endpoint. But a delay of a
few onsets buys the real thing: each onset's influence on earlier knots decays
like `γⁿ` with the number of onsets in between, so a knot 3–5 onsets back has
essentially converged to the final batch spline ("fixed-lag" smoothing).

`TempoSmoother` implements this over a sliding window of recent onsets. Per
onset it re-runs, over the window (O(window) 2×2 arithmetic):

1. a **forward Kalman pass** — the same state-space model as §3, with actual
   covariances (`q` derived from the one `memory` knob via the tracking index
   `Λ = β/√(1−α) = (1−γ)²/γ`, `ρ = 1`);
2. a **backward Rauch–Tung–Striebel pass**, which folds each later knot's
   information into the earlier ones: the posterior `(position, tempo)` at
   every onset time;
3. **between** knots, the posterior mean is the **cubic Hermite bridge**
   matching both knot states — so the full curve is the C¹ piecewise-cubic,
   i.e. the (windowed) cubic smoothing spline itself. Exact timing ⇒ the spline
   is exactly the constant-tempo line (unit-tested, like §5).

Two consumers:

- **Visualization**: the smoothed BPM curve is drawn solid (re-sent per onset —
  its recent end visibly bends into place as new onsets land); the causal
  per-frame estimate stays as a faint trace, carrying the leading edge from the
  last onset to "now", where no spline can exist yet.
- **Scroll anchor**: the view no longer follows the causal extrapolation but
  the smoothed position `smoothDelayIntervals` (= 4) onsets back — C¹, so the
  pan speed is continuous too. The delay is affordable because the anchor
  doesn't need to sit at the playhead: it rests at the **first third** of the
  view (`anchorFrac = ⅓`), and the notes actually being played float ahead of
  it, around mid-view. The auto-zoom keeps both edges honest: the trailing
  active hand inside the left third, the leading hand's causal "now" inside the
  right two thirds. Past the spline's end (the performer paused), the anchor
  cross-fades to the causal, coast-damped state so it eases to a stop.

  Even at that lag the raw anchor still jumps a little at each onset — the
  spline's tail is re-fitted (`γ⁴ ≈ 13 %` of the correction) and, more, the
  delay itself is `smoothDelayIntervals · Δ̄` where the cadence EMA `Δ̄` moves on
  every onset of a mixed rhythm. Each jump is absorbed into a decaying offset
  (the §6.2 trick, applied a second time at the anchor level), so the
  *displayed* anchor is exactly continuous; a repeat's intended backward snap
  is exempted.

---

## 8. Parameters

| symbol | name              | current value | meaning                                     |
| ------ | ----------------- | ------------- | ------------------------------------------- |
| `γ`    | `memory` (ctor)   | 0.6           | smoothing/lag; `α=1−γ²`, `β=(1−γ)²`         |
| —      | `coastTrigger`    | 15            | overdue threshold, in inter-onset intervals |
| —      | `coastFactor`     | 0.9           | coast decay τ as a fraction of `Δ̄`          |
| —      | `offsetDecayMs`   | 1000          | continuity-offset decay τ (ms)              |
| —      | `ticksPerQuarter` | 480           | ticks→BPM conversion (MuseScore division)   |
| —      | `smoothDelayIntervals` | 4        | scroll-anchor lag on the spline, in onsets  |
| —      | `anchorOffsetDecayMs` | 1000      | anchor continuity-offset decay τ (ms)       |
| —      | `anchorFrac`      | ⅓             | where the scroll anchor rests in the view   |

Only `γ` is the estimator knob (shared by filter and smoother); the others
govern the idle/transient/viewport policies.

---

## 9. Provenance & references

- **Origin.** Ported from the Christophone _tempo-driven controller_, which fit a
  recency-weighted least-squares line `musicalTime ≈ slope·t + intercept` to
  recent onsets. The α–β filter is the recursive, fading-memory equivalent of
  that weighted line fit.
- **Smoothing spline ↔ Kalman / integrated Wiener process** duality (Wahba;
  Kalman & Bucy) — §2(d): why "fit a smooth curve" becomes a Kalman filter.
- **α–β (and α–β–γ) tracking filters** from the radar-tracking literature; the
  fading-memory `γ` parameterization with `α=1−γ²`, `β=(1−γ)²`.
- **Expressive-timing models** (for the eventual rhythm grading, not this
  estimator): Friberg & Sundberg's locomotion model of final ritardandi; Todd's
  kinematic phrase-arch model.
