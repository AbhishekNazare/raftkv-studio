#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONTROL_PLANE_URL="${CONTROL_PLANE_URL:-http://127.0.0.1:8090}"
CONTROL_PLANE_PORT="${CONTROL_PLANE_URL##*:}"

"${ROOT_DIR}/scripts/stop_dev.sh" >/dev/null 2>&1 || true

cleanup() {
  kill "${UI_PID:-}" "${CONTROL_PLANE_PID:-}" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

export PYTHONPATH="${ROOT_DIR}/control-plane/src"
python3 -m raftkv_control_plane.main --port "${CONTROL_PLANE_PORT}" &
CONTROL_PLANE_PID=$!

for _ in {1..30}; do
  if curl --fail --silent --max-time 1 "${CONTROL_PLANE_URL}/health" >/dev/null 2>&1; then
    break
  fi
  sleep 0.2
done

if ! curl --fail --silent --max-time 1 "${CONTROL_PLANE_URL}/health" >/dev/null 2>&1; then
  echo "Control plane did not become healthy at ${CONTROL_PLANE_URL}" >&2
  exit 1
fi

echo "Control plane: ${CONTROL_PLANE_URL}"
echo "UI: http://127.0.0.1:5173"

(
  cd "${ROOT_DIR}/ui"
  CONTROL_PLANE_URL="${CONTROL_PLANE_URL}" npm run dev
) &
UI_PID=$!

wait "${UI_PID}"
