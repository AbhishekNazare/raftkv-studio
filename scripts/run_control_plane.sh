#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONPATH="${ROOT_DIR}/control-plane/src"

HOST="127.0.0.1"
PORT="8090"
ARGS=("$@")

for ((i = 0; i < ${#ARGS[@]}; i++)); do
  case "${ARGS[$i]}" in
    --host)
      if ((i + 1 < ${#ARGS[@]})); then
        HOST="${ARGS[$((i + 1))]}"
      fi
      ;;
    --host=*)
      HOST="${ARGS[$i]#--host=}"
      ;;
    --port)
      if ((i + 1 < ${#ARGS[@]})); then
        PORT="${ARGS[$((i + 1))]}"
      fi
      ;;
    --port=*)
      PORT="${ARGS[$i]#--port=}"
      ;;
  esac
done

if curl --fail --silent --max-time 1 "http://${HOST}:${PORT}/health" >/dev/null 2>&1; then
  echo "raftkv-control-plane is already running at http://${HOST}:${PORT}"
  exit 0
fi

if python3 -c "import socket; s=socket.socket(); s.settimeout(0.5); raise SystemExit(0 if s.connect_ex(('${HOST}', ${PORT})) == 0 else 1)" >/dev/null 2>&1; then
  echo "Port ${PORT} on ${HOST} is already in use, but /health did not respond." >&2
  echo "Stop the existing process or run: $0 --port 8091" >&2
  exit 1
fi

python3 -m raftkv_control_plane.main "$@"
