#!/usr/bin/env bash
set -euo pipefail

echo "=== Demo: Quorum Log Replication ==="
echo "[1] Running backend replication tests."
echo "[2] This proves writes commit with 3/3 and 2/3 acknowledgements, and do not commit with 1/3."

./scripts/test_backend.sh

echo "PASS: quorum replication behavior is covered by raftkv_replication_test."
echo "NOTE: live Docker replication will be wired after the gRPC node server phase."

