#!/usr/bin/env python3
"""Sample every thread of a running Orchestrion from /proc, for perf_trace_report.py.

    python3 buildscripts/perf_thread_sampler.py /dev/shm/orchestrion-threads.log [--pid PID] [--seconds S]

Without --pid, the one running process named Orchestrion is sampled; with
several running (a debug instance and the one under test, say) they are listed
and you pick with --pid. Runs until Ctrl-C or --seconds.

Every 20 ms, per thread: scheduler state (R running, S sleeping, D blocked in
the kernel — a filesystem write, say), CPU ticks, block-I/O delay, run and
run-queue-wait time, and the kernel wait channel. Timestamps are
CLOCK_MONOTONIC in microseconds, the same axis as ORCHESTRION_PERF_LOG, so
`perf_trace_report.py trace.log --threads threads.log` can say what each thread
was doing at a stall. Needs no privileges for your own processes.
"""
import argparse
import os
import sys
import time


def rd(path):
    try:
        with open(path) as f:
            return f.read()
    except OSError:
        return ""


def find_orchestrion():
    found = []
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        if rd(f"/proc/{entry}/comm").strip() == "Orchestrion":
            try:
                exe = os.readlink(f"/proc/{entry}/exe")
            except OSError:
                exe = "?"
            found.append((int(entry), exe))
    return found


ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
ap.add_argument("out", help="where to write the samples (use /dev/shm)")
ap.add_argument("--pid", type=int, help="process to sample (default: the one Orchestrion running)")
ap.add_argument("--seconds", type=float, default=1e9, help="stop after this long (default: until Ctrl-C)")
args = ap.parse_args()

pid = args.pid
if pid is None:
    found = find_orchestrion()
    if len(found) != 1:
        if not found:
            sys.exit("no running process named Orchestrion; pass --pid")
        print("several Orchestrion processes; pass --pid with one of:", file=sys.stderr)
        for p, exe in found:
            print(f"  --pid {p}   {exe}", file=sys.stderr)
        sys.exit(1)
    pid = found[0][0]
    print(f"sampling pid {pid} ({found[0][1]}) -> {args.out}; Ctrl-C to stop", file=sys.stderr)

out = open(args.out, "w")
duration = args.seconds
base = f"/proc/{pid}/task"
names = {}


t_end = time.monotonic() + duration
try:
    while time.monotonic() < t_end:
        now_us = time.monotonic_ns() // 1000
        try:
            tids = os.listdir(base)
        except OSError:
            break
        for tid in tids:
            if tid not in names:
                names[tid] = rd(f"{base}/{tid}/comm").strip().replace(" ", "_") or "?"
            stat = rd(f"{base}/{tid}/stat")
            if not stat:
                continue
            fields = stat[stat.rfind(")") + 2:].split()
            state = fields[0]
            cpu_ticks = int(fields[11]) + int(fields[12])
            blkio = int(fields[39]) if len(fields) > 39 else -1
            sched = rd(f"{base}/{tid}/schedstat").split()
            run_ns, wait_ns = (int(sched[0]), int(sched[1])) if len(sched) >= 2 else (-1, -1)
            wchan = rd(f"{base}/{tid}/wchan").strip() or "-"
            out.write(f"{now_us} {tid} {names[tid]} {state} {cpu_ticks} {blkio} {run_ns} {wait_ns} {wchan}\n")
        out.flush()
        time.sleep(0.02)
except KeyboardInterrupt:
    pass
out.close()
