#!/usr/bin/env python3
# This file is part of Orchestrion.
#
# Copyright (C) 2026 Matthieu Hodgkinson
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
"""Summarises a LatencyTrace (src/OrchestrionCommon/LatencyTrace.h).

Run the app with --latency-trace <dir> (made if needed; on Linux preferably
on tmpfs, e.g. /dev/shm/trace, so that writing it doesn't disturb what is
measured), play, quit, then:

    tools/latency_trace.py <dir> [--notes]

Steps (see src/OrchestrionSynthesis/gestureToSound.md):
  dispatched -> played    input thread to play thread; for the automatic
                          player's notes, how late it fired after the note
                          was due
  played     -> queued    the sequencer, until it queues the notes
  queued     -> sent      output thread (minus what the sequencer delayed
                          on purpose, an ornament's or a grace's due time)
  sent       -> received  kors to the audio worker, which polls every 16 ms
  received   -> rendered  wait for the next callback (the first starting
                          after received)
  total = dispatched -> rendered; the device's output latency comes on top
  and isn't measured.
"""

import bisect
import csv
import os
import sys


def percentiles(values):
    if not values:
        return "n/a"
    v = sorted(values)

    def p(q):
        return v[min(len(v) - 1, int(q * (len(v) - 1) + 0.5))]

    return "n=%4d  min %7.2f  p50 %7.2f  p90 %7.2f  p99 %7.2f  max %7.2f" % (
        len(v), v[0], p(0.5), p(0.9), p(0.99), v[-1])


def ms(ns):
    return ns / 1e6


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 1:
        sys.exit(__doc__)
    directory = args[0]
    show_notes = "--notes" in sys.argv

    meta = {}
    with open(os.path.join(directory, "meta.txt")) as f:
        for line in f:
            key, _, value = line.strip().partition("=")
            meta[key] = value
    rate = int(meta.get("sampleRate") or 0)

    with open(os.path.join(directory, "callbacks.csv")) as f:
        callbacks = [(int(r["start"]), int(r["end"]), int(r["frames"]))
                     for r in csv.DictReader(f)]
    starts = [c[0] for c in callbacks]

    with open(os.path.join(directory, "notes.csv")) as f:
        notes = list(csv.DictReader(f))

    print("Output: %s, %s Hz, %s frames requested" %
          (meta.get("driver"), meta.get("sampleRate"),
           meta.get("requestedFrames")))

    if callbacks:
        frames = sorted({c[2] for c in callbacks})
        print("\nCallbacks: %d, frames per callback: %s" %
              (len(callbacks), frames))
        intervals = [ms(b[0] - a[0]) for a, b in zip(callbacks, callbacks[1:])]
        print("  interval (ms)    " + percentiles(intervals))
        print("  processing (ms)  " +
              percentiles([ms(c[1] - c[0]) for c in callbacks]))
        if rate:
            loads = [(c[1] - c[0]) / 1e9 / (c[2] / rate) * 100
                     for c in callbacks if c[2]]
            print("  load (%)         " + percentiles(loads))
            late = [i for i, (a, b) in enumerate(zip(callbacks, callbacks[1:]))
                    if (b[0] - a[0]) / 1e9 > 1.5 * a[2] / rate]
            print("  intervals > 1.5 periods: %d" % len(late))

    groups = [(source, kind) for source in ["gesture", "auto"]
              for kind in ["on", "off"]]
    steps = {k: {g: [] for g in groups} for k in
             ["played", "queued", "sent (late)", "due delay", "received",
              "rendered", "total"]}
    incomplete = 0
    ambiguous = 0
    rows = []
    for n in notes:
        t = {k: int(n[k]) for k in
             ["dispatched", "played", "queued", "due", "sent", "received"]}
        group = (n.get("source", "gesture"), n["kind"])
        if not all(t[k] for k in
                   ["dispatched", "played", "queued", "sent", "received"]):
            incomplete += 1
            continue
        i = bisect.bisect_left(starts, t["received"])
        if i == len(starts):
            incomplete += 1
            continue
        rendered = starts[i]
        # Told while a callback ran: that one may have rendered it already.
        if i > 0 and callbacks[i - 1][1] > t["received"]:
            ambiguous += 1
        target = max(t["queued"], t["due"])
        d = {
            "played": ms(t["played"] - t["dispatched"]),
            "queued": ms(t["queued"] - t["played"]),
            "due delay": ms(target - t["queued"]),
            "sent (late)": ms(t["sent"] - target),
            "received": ms(t["received"] - t["sent"]),
            "rendered": ms(rendered - t["received"]),
            "total": ms(rendered - t["dispatched"]),
        }
        for k, v in d.items():
            steps[k][group].append(v)
        rows.append((n["id"], "%s-%s" % group, n["pitch"], d))

    print("\nNotes: %d traced, %d incomplete (no score, no sound, or "
          "after the last callback), %d told during a callback" %
          (len(notes), incomplete, ambiguous))
    for group in groups:
        if not steps["total"][group]:
            continue
        print("\n%s note-%s (ms)" % group)
        for k in ["played", "queued", "due delay", "sent (late)", "received",
                  "rendered", "total"]:
            print("  %-12s %s" % (k, percentiles(steps[k][group])))

    if show_notes:
        print("\nid source-kind pitch " + " ".join("%11s" % k for k in rows[0][3])
              if rows else "")
        for id_, kind, pitch, d in rows:
            print("%s %s %s " % (id_, kind, pitch) +
                  " ".join("%11.2f" % v for v in d.values()))


if __name__ == "__main__":
    main()
