#!/usr/bin/env bash
# Usage: tests/smoke.sh <build-dir>
set -euo pipefail
B="${1:-build}"
PORT=18080
PIDS=()
cleanup() { for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null || true; done; }
trap cleanup EXIT

"$B/procd" & PIDS+=($!)
sleep 0.3
"$B/metricsd" --port $PORT & PIDS+=($!)
"$B/sensord" --interval-ms 100 & PIDS+=($!)
sleep 2

curl -sf "localhost:$PORT/health" | grep -q ok
curl -sf "localhost:$PORT/metrics" | grep -q uptime_seconds
STATE=$(curl -sf "localhost:$PORT/state")
echo "$STATE"
echo "$STATE" | grep -q '"messages"'
MSGS=$(echo "$STATE" | grep -o '"messages": [0-9]*' | grep -o '[0-9]*$')
[ "$MSGS" -gt 5 ]
echo "SMOKE TEST PASSED ($MSGS messages)"
