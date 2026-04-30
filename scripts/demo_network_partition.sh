#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONPATH="${ROOT_DIR}/control-plane/src"

python3 - <<'PY'
from raftkv_control_plane.cluster_service import ClusterService

service = ClusterService()
result = service.run_demo("network-partition")

print("=== Demo: Network Partition ===")
print(f"old leader: {result['oldLeader']}")
print(f"new leader: {result['newLeader']}")
print(f"committed: {result['write']['committed']}")
print(f"value: {result['cluster']['kv'].get('split:test')}")

if not result["passed"]:
    raise SystemExit("FAIL: network partition demo did not pass")

print("PASS: majority-side leader committed while isolated old leader stepped down after heal.")
PY

