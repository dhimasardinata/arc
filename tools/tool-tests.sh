#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tools -p '*_test.py'

echo "arc tool tests: OK"
