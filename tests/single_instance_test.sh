#!/usr/bin/env bash
# Starts the app, then a second instance. The second must exit with status 0 and the first
# must log showPrimary. Usage: single_instance_test.sh /path/to/stickynotes
set -u
APP=${1:?path to the stickynotes binary}
if [ -z "${_STICKY_IN_BUS:-}" ]; then
    export _STICKY_IN_BUS=1
    exec dbus-run-session -- "$0" "$APP"
fi
export QT_QPA_PLATFORM=offscreen QT_FORCE_STDERR_LOGGING=1
export QT_LOGGING_RULES="stickynotes.activation=true"
export XDG_DATA_HOME=$(mktemp -d) XDG_CONFIG_HOME=$(mktemp -d)
LOG=$(mktemp)
"$APP" >"$LOG" 2>&1 &
FIRST=$!
trap 'kill $FIRST 2>/dev/null; rm -rf "$XDG_DATA_HOME" "$XDG_CONFIG_HOME" "$LOG"' EXIT

for _ in $(seq 1 50); do
    busctl --user list 2>/dev/null | grep -q "io.github.rupeshofficedata.StickyNotes" && break
    sleep 0.1
done
busctl --user list | grep -q "io.github.rupeshofficedata.StickyNotes" || { echo "FAIL: name never registered"; cat "$LOG"; exit 1; }

timeout 10 "$APP" >/dev/null 2>&1
STATUS=$?
[ "$STATUS" -eq 0 ] || { echo "FAIL: second launch exited with $STATUS"; exit 1; }

for _ in $(seq 1 30); do grep -q "showPrimary" "$LOG" && break; sleep 0.1; done
grep -q "showPrimary" "$LOG" || { echo "FAIL: first instance never logged showPrimary"; cat "$LOG"; exit 1; }
kill -0 $FIRST 2>/dev/null || { echo "FAIL: first instance died"; exit 1; }
echo "PASS"
