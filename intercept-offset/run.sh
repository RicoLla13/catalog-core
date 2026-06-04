#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_offset()
{
	rm -rf /tmp/intercept-offset-root
	mkdir -p /tmp/intercept-offset-root
	rm -f /tmp/intercept-offset-root/intercept-offset.txt
}

intercept_example_run "intercept-offset_qemu-x86_64" seed_intercept_offset
