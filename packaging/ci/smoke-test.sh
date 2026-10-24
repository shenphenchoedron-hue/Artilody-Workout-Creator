#!/usr/bin/env bash
# QML smoke test: start the app in screenshot mode, which uses temporary
# settings and a temporary Workouts folder with demo workouts, renders several
# views to PNG and quits by itself (see WATT_SCREENSHOT_DIR in src/main.cpp).
#
#   packaging/ci/smoke-test.sh "<path to executable>" "<screenshot dir>"
#
# Fails when
#   * the app does not exit within SMOKE_TIMEOUT seconds (default 120),
#   * the app exits with a non-zero status,
#   * the log contains QML errors/warnings (qrc:...qml:<line>),
#   * fewer than SMOKE_MIN_SHOTS (default 5) PNGs were produced.
#
# Works with Bash on Linux, macOS and Git Bash on Windows. All paths may
# contain spaces.
set -uo pipefail

bin="${1:?usage: smoke-test.sh <executable> [screenshot dir]}"
shots="${2:-${RUNNER_TEMP:-/tmp}/awc-shots}"
timeout_s="${SMOKE_TIMEOUT:-120}"
min_shots="${SMOKE_MIN_SHOTS:-5}"

if [ ! -f "$bin" ]; then
    echo "::error::Executable not found: $bin"
    exit 1
fi

work="$(mktemp -d)"
# Native Windows executables need Windows paths (Git Bash: /tmp -> C:/...).
if command -v cygpath >/dev/null 2>&1; then
    work="$(cygpath -m "$work")"
    shots="$(cygpath -m "$shots")"
fi
rm -rf "$shots"
mkdir -p "$shots"

export WATT_SCREENSHOT_DIR="$shots"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
export QT_FORCE_STDERR_LOGGING=1          # Windows GUI apps log to stderr, not OutputDebugString
export QT_LOGGING_RULES="${QT_LOGGING_RULES:-qt.qml.binding.removal.info=false}"

echo "Running: \"$bin\" (QT_QPA_PLATFORM=$QT_QPA_PLATFORM, QT_QUICK_BACKEND=${QT_QUICK_BACKEND:-default}, timeout ${timeout_s}s)"

if command -v timeout >/dev/null 2>&1; then
    timeout_cmd=(timeout --kill-after=10 "$timeout_s")
elif command -v gtimeout >/dev/null 2>&1; then
    timeout_cmd=(gtimeout --kill-after=10 "$timeout_s")
else
    # macOS without coreutils: SIGALRM after N seconds (exit status 142).
    timeout_cmd=(perl -e 'alarm shift; exec { $ARGV[0] } @ARGV or die "exec failed: $!"' "$timeout_s")
fi

start=$(date +%s)
"${timeout_cmd[@]}" "$bin" >"$work/stdout.log" 2>"$work/stderr.log"
rc=$?
echo "Exit code: $rc after $(( $(date +%s) - start ))s"

echo "----- stdout -----"; cat "$work/stdout.log"
echo "----- stderr -----"; cat "$work/stderr.log"
echo "------------------"
cp "$work/stdout.log" "$work/stderr.log" "$shots/" 2>/dev/null || true

fail=0
if [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ] || [ "$rc" -eq 142 ]; then
    echo "::error::App did not finish within ${timeout_s}s"
    fail=1
elif [ "$rc" -ne 0 ]; then
    echo "::error::App exited with status $rc"
    fail=1
fi

qml_errors="$(cat "$work/stderr.log" "$work/stdout.log" | grep -E 'qrc:.*\.qml:[0-9]+' || true)"
if [ -n "$qml_errors" ]; then
    echo "::error::QML errors/warnings found:"
    echo "$qml_errors"
    fail=1
fi

count=$(find "$shots" -maxdepth 1 -name '*.png' | wc -l | tr -d ' ')
echo "Screenshots produced: $count (expected >= $min_shots)"
ls -l "$shots"
if [ "$count" -lt "$min_shots" ]; then
    echo "::error::Only $count of $min_shots screenshots were produced"
    fail=1
fi

rm -rf "$work"
exit $fail
