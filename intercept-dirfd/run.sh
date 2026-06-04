#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_dirfd()
{
	rm -rf /tmp/intercept-dirfd-root
	mkdir -p /tmp/intercept-dirfd-root
	rm -f /tmp/intercept-dirfd-root/intercept-dirfd.txt
}

intercept_example_run "intercept-dirfd_qemu-x86_64" seed_intercept_dirfd
