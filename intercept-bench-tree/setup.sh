#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

if [ "${1:-}" = clean ]; then
	intercept_example_clean
	rm -f workdir/.last-config-mode
	exit 0
fi

intercept_example_setup_workdir
