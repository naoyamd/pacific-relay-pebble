#!/bin/bash
# Use the SDK Python runtime so each fixture retains a single clock connection.
set -euo pipefail
task_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec "${PEBBLE_PYTHON:-python3}" "$task_root/tools/capture_atlas.py"
