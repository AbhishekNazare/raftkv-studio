#!/usr/bin/env bash
set -euo pipefail

echo "=== Demo: Leader Election Safety ==="
echo "[1] Running deterministic RaftNode election test."
echo "[2] This proves a candidate can collect majority votes and exactly one leader is recorded in the in-process cluster."

./scripts/test_backend.sh

echo "PASS: leader election behavior is covered by raftkv_node_test."
echo "NOTE: live Docker leader election will be wired after the gRPC node server phase."

