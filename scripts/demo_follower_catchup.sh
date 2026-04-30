#!/usr/bin/env bash
set -euo pipefail

echo "=== Demo: Follower Catch-Up ==="
echo "[1] Running failover and catch-up tests."
echo "[2] This proves a follower can miss committed entries, restart, receive missing entries, and apply them."

./scripts/test_backend.sh

echo "PASS: follower catch-up behavior is covered by raftkv_failover_test."
echo "NOTE: live Docker catch-up will be wired after the gRPC node server phase."

