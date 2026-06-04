#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_simple()
{
	rm -rf /tmp/intercept-simple-root
	mkdir -p /tmp/intercept-simple-root
	printf '%s\n' "seeded by intercept-simple/run.sh" >/tmp/intercept-simple-root/existing.txt
	rm -f /tmp/intercept-simple-root/missing.txt
}

intercept_example_run "intercept-simple_qemu-x86_64" seed_intercept_simple
