#!/usr/bin/env bash
#
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
#
# Runs one testflow script (testflowscripts/) in a throwaway environment and
# exits with its result (0 passed, 1 failed, 2 couldn't run, 124 timed out):
#
#   tools/run_testflow.sh <app> <script> [--record <video.mp4 or dir/>] [--speed <speed>] [--log <file>]
#
# <app>: the Orchestrion executable, an extracted AppImage's AppRun, or an
# .AppImage (extracted, so FUSE isn't needed). <script>: a file, or a name in
# testflowscripts/. --log keeps the app's log (with the scripts' api.log
# output), which is otherwise deleted with the rest of the environment.
#
# The environment: a fresh profile; a virtual X display (Xvfb, with openbox if
# installed, without which the window can't go full screen); a silent audio
# output that runs in real time, like a sound card (a null sink of the running
# PulseAudio/PipeWire server, or of a PulseAudio daemon started for the run -
# ALSA's null device won't do, it starves the audio engine). With --record,
# ffmpeg records the display and that sink into a video.
#
# Directives, in the comments a script starts with:
#   @testflow record-from <step name>    the video starts when that step starts
#   @testflow record-after <step name>   the video starts when that step ends
#   @testflow test                       a test, not a tutorial (the app lists
#                                        the other scripts as tutorials)
# Recorded, the app starts in full screen.
#
# Linux only. Needs Xvfb, pactl, and for --record ffmpeg.

set -euo pipefail

usage() {
    sed -n '/^# Runs one/,/^# Linux only/p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
}

[ $# -ge 2 ] || usage
APP=$(realpath "$1")
SCRIPT=$2
shift 2
RECORD=""
SPEED=""
LOG=""
while [ $# -gt 0 ]; do
    case "$1" in
        --record) RECORD=$2; shift 2 ;;
        --speed) SPEED=$2; shift 2 ;;
        --log) LOG=$(realpath -m "$2"); shift 2 ;;
        *) usage ;;
    esac
done

# A directory (ending with /) gets a video named after the script
[[ "$RECORD" != */ ]] || RECORD="$RECORD$(basename "$SCRIPT" .js).mp4"
[ -z "$RECORD" ] || RECORD=$(realpath -m "$RECORD")

REPO=$(cd "$(dirname "$0")/.." && pwd)
SCRIPTS_DIR="$REPO/testflowscripts"

# The script's directives
SCRIPT_FILE=""
for candidate in "$SCRIPT" "$SCRIPTS_DIR/$SCRIPT" "$SCRIPTS_DIR/$SCRIPT.js"; do
    [ -f "$candidate" ] && { SCRIPT_FILE=$candidate; break; }
done
RECORD_AFTER=""
RECORD_FROM=""
if [ -n "$SCRIPT_FILE" ]; then
    # The `@testflow <directive> [<argument>]` lines of the comments before the code
    while read -r directive argument; do
        case "$directive" in
            record-after) RECORD_AFTER=$argument ;;
            record-from) RECORD_FROM=$argument ;;
            test) ;;
            *) echo "unknown directive @testflow $directive in $SCRIPT_FILE"; exit 2 ;;
        esac
    done < <(awk '
        inComment { if (match($0, /@testflow[ \t]+/)) print substr($0, RSTART + RLENGTH)
                    if ($0 ~ /\*\//) inComment = 0; next }
        /^[ \t]*$/ || /^[ \t]*\/\// { next }
        /^[ \t]*\/\*/ { if ($0 !~ /\*\//) inComment = 1; next }
        { exit }' "$SCRIPT_FILE")
fi
WIDTH=1920
HEIGHT=1080
TIMEOUT_S=${TESTFLOW_TIMEOUT_S:-300}

WORK=$(mktemp -d)
PIDS=()
SINK_MODULE=""
cleanup() {
    for pid in "${PIDS[@]}"; do kill "$pid" 2>/dev/null || true; done
    wait 2>/dev/null || true
    [ -z "$SINK_MODULE" ] || pactl unload-module "$SINK_MODULE" 2>/dev/null || true
    rm -rf "$WORK"
}
trap cleanup EXIT

# The app
if [[ "$APP" == *.AppImage ]]; then
    (cd "$WORK" && "$APP" --appimage-extract > /dev/null)
    APP="$WORK/squashfs-root/AppRun"
fi
[ -x "$APP" ] || { echo "not an executable: $APP"; exit 2; }

# A display
exec 3< <(Xvfb -displayfd 1 -screen 0 "${WIDTH}x${HEIGHT}x24" -nolisten tcp 2> "$WORK/xvfb.log")
PIDS+=($!)
read -r -t 10 DISPLAY_NUM <&3 || { echo "Xvfb didn't start"; cat "$WORK/xvfb.log"; exit 2; }
export DISPLAY=":$DISPLAY_NUM"
if command -v openbox > /dev/null; then
    openbox > "$WORK/openbox.log" 2>&1 &
    PIDS+=($!)
fi

# A silent audio output, paced like a sound card
if ! pactl info > /dev/null 2>&1; then
    command -v pulseaudio > /dev/null || { echo "no sound server, and no pulseaudio to start one"; exit 2; }
    pulseaudio --daemonize=no --exit-idle-time=-1 --disallow-exit > "$WORK/pulseaudio.log" 2>&1 &
    PIDS+=($!)
    for _ in $(seq 50); do pactl info > /dev/null 2>&1 && break; sleep 0.1; done
fi
SINK="orchestrion_testflow_$$"
SINK_MODULE=$(pactl load-module module-null-sink "sink_name=$SINK" "sink_properties=device.description=$SINK")
export PULSE_SINK="$SINK"     # ALSA's pulse device
export PIPEWIRE_NODE="$SINK"  # ALSA's pipewire device
if aplay -L 2> /dev/null | grep -qx pipewire; then
    AUDIO_DEVICE=pipewire
else
    AUDIO_DEVICE=pulse
fi

# A fresh profile: no hints over the score, and the audio to the null sink
export XDG_CONFIG_HOME="$WORK/config" XDG_DATA_HOME="$WORK/data" \
       XDG_CACHE_HOME="$WORK/cache" XDG_STATE_HOME="$WORK/state"
mkdir -p "$XDG_CONFIG_HOME/Unknown Organization"
for name in Orchestrion OrchestrionDevelopment; do
    cat > "$XDG_CONFIG_HOME/Unknown Organization/$name.ini" <<EOF
[General]
AUDIO_DEVICE=$AUDIO_DEVICE
SCORE_SHELF_HINT_DISMISSED=true
EOF
done

# Testflow's paths: scripts (and their require("steps/...")), and reports
export MUSE_TESTFLOW_SCRIPTS_PATH="$SCRIPTS_DIR"
export MUSE_TESTFLOW_DATA_PATH="$WORK/testflow"

# Recorded in full, then cut at the record-after step if any. Both inputs'
# timestamps are the wall clock (-copyts): ffmpeg would otherwise start each at
# zero, and the audio capture starts later than the screen's (by most of a
# second on CI), which would put the sound ahead of the picture.
RAW_VIDEO="$WORK/raw.mp4"
if [ -n "$RECORD" ]; then
    mkdir -p "$(dirname "$RECORD")"
    ffmpeg -loglevel error -y \
        -f x11grab -video_size "${WIDTH}x${HEIGHT}" -framerate 30 -i "$DISPLAY" \
        -f pulse -i "$SINK.monitor" \
        -copyts \
        -c:v libx264 -preset veryfast -pix_fmt yuv420p -c:a aac -b:a 160k \
        "$RAW_VIDEO" < /dev/null > "$WORK/ffmpeg.log" 2>&1 &
    FFMPEG_PID=$!
fi

ARGS=(--test-case "$SCRIPT")
[ -z "$SPEED" ] || ARGS+=(--test-case-speed "$SPEED")
# The video shows the window filling the screen
[ -z "$RECORD" ] || ARGS+=(--full-screen)
rc=0
timeout -k 15 "$TIMEOUT_S" "$APP" "${ARGS[@]}" > "$WORK/app.log" 2>&1 || rc=$?
[ -z "$LOG" ] || { mkdir -p "$(dirname "$LOG")"; cp "$WORK/app.log" "$LOG"; }

if [ -n "$RECORD" ]; then
    kill -INT "$FFMPEG_PID" 2>/dev/null || true
    # Stopped this way, ffmpeg exits with 255 even when the video is fine
    wait "$FFMPEG_PID" || true
    [ -s "$RAW_VIDEO" ] || { echo "--- ffmpeg failed:"; cat "$WORK/ffmpeg.log"; }
    # The wall-clock time of the first frame
    RAW_START_S=$(ffprobe -v error -show_entries format=start_time -of csv=p=0 "$RAW_VIDEO")
    # The time of the record-from step's start, or of the record-after one's
    # end, from the app's log
    stepTime() { # event name
        grep -F "] step $1 at=" "$WORK/app.log" \
            | awk -v name="$2" '{ at = $4; sub(/^at=/, "", at); $1 = $2 = $3 = $4 = ""; sub(/^ +/, ""); if ($0 == name) { print at; exit } }'
    }
    CUT_MS=""
    if [ -n "$RECORD_FROM" ]; then
        CUT_MS=$(stepTime started "$RECORD_FROM")
        [ -n "$CUT_MS" ] || echo "step \"$RECORD_FROM\" (record-from) didn't start: the video is the whole run"
    elif [ -n "$RECORD_AFTER" ]; then
        CUT_MS=$(stepTime finished "$RECORD_AFTER")
        [ -n "$CUT_MS" ] || echo "step \"$RECORD_AFTER\" (record-after) didn't finish: the video is the whole run"
    fi
    # Written anew, its timestamps from zero (the streams keeping their offset)
    if [ -n "$CUT_MS" ]; then
        CUT_S=$(awk -v end="$CUT_MS" -v start="$RAW_START_S" 'BEGIN { s = end / 1000 - start; printf "%.3f", (s > 0 ? s : 0) }')
        ffmpeg -loglevel error -y -ss "$CUT_S" -i "$RAW_VIDEO" \
            -c:v libx264 -preset veryfast -pix_fmt yuv420p -c:a aac -b:a 160k "$RECORD"
    else
        ffmpeg -loglevel error -y -i "$RAW_VIDEO" -c copy "$RECORD"
    fi
    echo "recorded $RECORD"
fi

grep '^\[testflow\]' "$WORK/app.log" || { echo "--- app log (tail):"; tail -40 "$WORK/app.log"; }
shopt -s nullglob
for report in "$MUSE_TESTFLOW_DATA_PATH"/reports/*.txt; do
    echo "--- $(basename "$report")"
    cat "$report"
done
[ "$rc" -ne 124 ] || echo "timed out after ${TIMEOUT_S}s"
exit "$rc"
