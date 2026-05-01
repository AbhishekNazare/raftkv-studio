#!/usr/bin/env bash
set -euo pipefail

PORTS=(5173 5174 5175 5176 8090 8091)
PIDS=()

for port in "${PORTS[@]}"; do
  while IFS= read -r pid; do
    if [[ -n "${pid}" ]]; then
      PIDS+=("${pid}")
    fi
  done < <(lsof -tiTCP:"${port}" -sTCP:LISTEN 2>/dev/null || true)
done

if [[ ${#PIDS[@]} -eq 0 ]]; then
  echo "No RaftKV dev ports are active."
  exit 0
fi

UNIQUE_PIDS="$(printf "%s\n" "${PIDS[@]}" | sort -u)"

while IFS= read -r pid; do
  kill "${pid}" 2>/dev/null || true
done <<< "${UNIQUE_PIDS}"

echo "Stopped processes on RaftKV dev ports: ${UNIQUE_PIDS//$'\n'/ }"
