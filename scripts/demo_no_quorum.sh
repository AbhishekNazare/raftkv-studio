#!/usr/bin/env bash
set -euo pipefail

echo "=== Demo: No Quorum ==="
echo "[1] Running replication tests."
echo "[2] This proves a leader with only 1/3 acknowledgements does not commit or apply the write."

./scripts/test_backend.sh

echo "PASS: no-quorum behavior is covered by raftkv_replication_test."
echo "NOTE: live Docker no-quorum demo will be wired after the gRPC node server phase."

