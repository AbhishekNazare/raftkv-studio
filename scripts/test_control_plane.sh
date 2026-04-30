#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONPATH="${ROOT_DIR}/control-plane/src"

python3 -m unittest discover -s "${ROOT_DIR}/control-plane/tests" -p "test_*.py"

