#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$SCRIPT_DIR/../scripts/capture-benchmark-results.sh" "$SCRIPT_DIR" "${1:-}"
