#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_rw()
{
	rm -rf /tmp/intercept-rw-root
	mkdir -p /tmp/intercept-rw-root
	rm -f /tmp/intercept-rw-root/intercept-rw.txt
}

intercept_example_run "intercept-rw_qemu-x86_64" seed_intercept_rw
