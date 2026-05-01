#!/usr/bin/env bash
set -euo pipefail

PYTHONPATH=control-plane/src python3 - <<'PY'
from raftkv_control_plane.cluster_service import ClusterService

service = ClusterService()
result = service.run_demo("snapshot-install")

if not result["passed"]:
    raise SystemExit("FAIL: snapshot install demo did not pass")

print("PASS: snapshot install demo passed.")
print(f"snapshotIndex={result['snapshotIndex']}")
print(f"compactedLogStartIndex={result['compactedLogStartIndex']}")
PY
