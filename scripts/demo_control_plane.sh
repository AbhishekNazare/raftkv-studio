#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONPATH="${ROOT_DIR}/control-plane/src"

python3 - <<'PY'
from raftkv_control_plane.cluster_service import ClusterService

service = ClusterService()

for scenario in ["no-quorum", "leader-failover", "network-partition"]:
    result = service.run_demo(scenario)
    print(f"{scenario}: {'PASS' if result['passed'] else 'FAIL'}")
    if not result["passed"]:
        raise SystemExit(1)

print("PASS: all control-plane demo scenarios passed.")
PY
