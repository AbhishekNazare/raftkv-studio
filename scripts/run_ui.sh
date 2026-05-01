#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -z "${CONTROL_PLANE_URL:-}" ]]; then
  if curl --fail --silent --max-time 1 "http://127.0.0.1:8090/health" >/dev/null 2>&1; then
    export CONTROL_PLANE_URL="http://127.0.0.1:8090"
  elif curl --fail --silent --max-time 1 "http://127.0.0.1:8091/health" >/dev/null 2>&1; then
    export CONTROL_PLANE_URL="http://127.0.0.1:8091"
  else
    export CONTROL_PLANE_URL="http://127.0.0.1:8090"
    echo "No control plane detected. Start one with ./scripts/run_control_plane.sh." >&2
  fi
fi

echo "UI proxy target: ${CONTROL_PLANE_URL}"

cd "${ROOT_DIR}/ui"
npm run dev
