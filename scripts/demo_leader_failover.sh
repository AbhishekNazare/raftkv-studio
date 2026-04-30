#!/usr/bin/env bash
set -euo pipefail

echo "=== Demo: Leader Failover ==="
echo "[1] Running failover tests."
echo "[2] This proves a committed value remains available after a new leader is elected."

./scripts/test_backend.sh

echo "PASS: leader failover behavior is covered by raftkv_failover_test."
echo "NOTE: live Docker failover will be wired after the gRPC node server phase."

