#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_fcntl()
{
	rm -rf /tmp/intercept-fcntl-root
	mkdir -p /tmp/intercept-fcntl-root
	rm -f /tmp/intercept-fcntl-root/intercept-fcntl.txt
}

intercept_example_run "intercept-fcntl_qemu-x86_64" seed_intercept_fcntl
