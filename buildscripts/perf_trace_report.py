#!/usr/bin/env python3
"""Summarise an ORCHESTRION_PERF_LOG trace (see src/OrchestrionCommon/PerfTrace.h).

    python3 buildscripts/perf_trace_report.py /path/to/perf.log [--stall-ms 25] [--late-ms 5]

Prints per-probe statistics, then every stall / late onset / audio dropout with
what the other threads were doing around it (+-50 ms), so a hiccup can be
attributed rather than guessed at.
"""
import argparse
import bisect
import statistics
import sys
from collections import defaultdict

def parse(path):
    events = []  # (t_us, tag, name, value_us, extra)
    with open(path, errors="replace") as f:
        for line in f:
            parts = line.rstrip("\n").split(" ", 4)
            if len(parts) < 4:
                continue
            try:
                t = int(parts[0]); v = int(parts[3])
            except ValueError:
                continue
            events.append((t, parts[1], parts[2], v, parts[4] if len(parts) > 4 else ""))
    events.sort(key=lambda e: e[0])
    return events

def parse_threads(path):
    """tid -> [(t_us, name, state, cpu_ticks, blkio, run_ns, wait_ns, wchan)]"""
    threads = {}
    with open(path, errors="replace") as f:
        for line in f:
            tok = line.split()
            if len(tok) < 9:
                continue
            threads.setdefault(tok[1], []).append((int(tok[0]), tok[2], tok[3], int(tok[4]), int(tok[5]), int(tok[6]), int(tok[7]), tok[8]))
    return threads


def thread_context(threads, t, window_us):
    """Threads that were blocked (D), starved (run-queue wait) or busy around t."""
    out = []
    for tid, rows in threads.items():
        ts = [r[0] for r in rows]
        lo = bisect.bisect_left(ts, t - window_us)
        hi = bisect.bisect_right(ts, t + window_us)
        if hi - lo < 2:
            continue
        seg = rows[lo:hi]
        cpu_ms = (seg[-1][3] - seg[0][3]) * 10
        wait_ms = (seg[-1][6] - seg[0][6]) / 1e6
        blkio = seg[-1][4] - seg[0][4]
        states = "".join(r[2] for r in seg)
        wchans = sorted({r[7] for r in seg} - {"0", "-"})
        if "D" in states or wait_ms > 5 or blkio > 0 or cpu_ms >= 30:
            out.append(f"      thread {tid} {seg[0][1]}: states={states} cpu={cpu_ms}ms rq-wait={wait_ms:.1f}ms blkio={blkio} wchan={wchans}")
    return out


def pct(xs, p):
    xs = sorted(xs)
    return xs[min(len(xs) - 1, int(p * len(xs)))]

def fmt_ms(us):
    return f"{us / 1000:.1f}ms"

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--stall-ms", type=float, default=25.0, help="frame gap counted as a GUI stall")
    ap.add_argument("--late-ms", type=float, default=5.0, help="onset lateness worth listing")
    ap.add_argument("--context-ms", type=float, default=50.0)
    ap.add_argument("--max-list", type=int, default=40)
    ap.add_argument("--threads", help="perf_thread_sampler.py output taken alongside the trace")
    ap.add_argument("--skip-s", type=float, default=3.0, help="ignore this many seconds after the first event (start-up: the audio buffer is empty until the worker runs)")
    args = ap.parse_args()
    threads = parse_threads(args.threads) if args.threads else {}

    events = parse(args.path)
    if not events:
        sys.exit("no events parsed")
    t0 = events[0][0]
    events = [e for e in events if e[0] >= t0 + int(args.skip_s * 1e6)]
    if not events:
        sys.exit("nothing after the skipped start-up")
    span_s = (events[-1][0] - t0) / 1e6
    print(f"{len(events)} events over {span_s:.1f} s\n")

    by_name = defaultdict(list)
    for t, tag, name, v, extra in events:
        by_name[(tag, name)].append(v)
    print(f"{'probe':28s} {'count':>7s} {'/s':>7s} {'median':>9s} {'p99':>9s} {'max':>9s}")
    for (tag, name), xs in sorted(by_name.items()):
        print(f"{tag + ' ' + name:28s} {len(xs):7d} {len(xs) / span_s:7.1f} {fmt_ms(statistics.median(xs)):>9s} {fmt_ms(pct(xs, 0.99)):>9s} {fmt_ms(max(xs)):>9s}")

    # Audio health at a glance.
    xruns = by_name.get(("audio", "xrun"), [])
    empties = by_name.get(("audio", "pop_empty"), [])
    shorts = by_name.get(("audio", "pop_short"), [])
    print(f"\naudio: {len(xruns)} device underruns (xrun), {len(empties)} empty pops (silence), {len(shorts)} short pops (stale samples)")
    proc = by_name.get(("worker", "process"), [])
    if proc:
        # Real time per rendered block is in the extra field; compare cost to budget.
        frames = [int(e[4].split("=")[1]) for e in events if e[1] == "worker" and e[2] == "process" and e[4].startswith("frames=")]
        if frames:
            print(f"worker: render step {statistics.median(frames):.0f} frames; process cost p99 {fmt_ms(pct(proc, 0.99))}, max {fmt_ms(max(proc))} (budget at 44.1 kHz: {statistics.median(frames) / 44.1:.1f}ms per step)")

    times = [e[0] for e in events]

    def context(t, exclude_idx):
        lo = bisect.bisect_left(times, t - int(args.context_ms * 1000))
        hi = bisect.bisect_right(times, t + int(args.context_ms * 1000))
        out = []
        for i in range(lo, hi):
            if i == exclude_idx:
                continue
            tt, tag, name, v, extra = events[i]
            if name == "writei":
                continue  # a blocking wait for the device, not work
            if name == "frame_gap":
                continue  # idle or stalled, can't tell; heartbeat_late says
            if name == "heartbeat_late" and v < 5000:
                continue
            if v >= 2000 or name in ("xrun", "pop_empty", "pop_short", "doLayout", "goto_tick", "heartbeat_late"):
                out.append(f"      {(tt - t) / 1000:+7.1f}ms {tag} {name} {fmt_ms(v)} {extra}")
        return out

    def list_incidents(title, predicate):
        hits = [(i, e) for i, e in enumerate(events) if predicate(e)]
        print(f"\n== {title}: {len(hits)}")
        for i, (t, tag, name, v, extra) in hits[: args.max_list]:
            print(f"  t=+{(t - t0) / 1e6:8.3f}s {tag} {name} {fmt_ms(v)} {extra}")
            for line in context(t, i):
                print(line)
            for line in thread_context(threads, t, int(args.context_ms * 1000)):
                print(line)
        if len(hits) > args.max_list:
            print(f"  ... {len(hits) - args.max_list} more")

    # The heartbeat's lateness is the GUI event loop's latency proper; a long
    # frame gap alone may just be an idle page (nothing requested a frame).
    list_incidents(f"GUI stalls (heartbeat > {args.stall_ms:g} ms late)",
                   lambda e: e[1] == "gui" and e[2] == "heartbeat_late" and e[3] > args.stall_ms * 1000)
    list_incidents(f"late onsets (> {args.late_ms:g} ms)",
                   lambda e: e[2] in ("autoplay_late", "replay_late", "wake_late") and e[3] > args.late_ms * 1000)
    list_incidents("audio dropouts", lambda e: e[2] in ("xrun", "pop_empty", "pop_short"))

if __name__ == "__main__":
    main()
