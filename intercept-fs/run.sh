#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_fs()
{
	rm -rf /tmp/intercept-fs-root
	mkdir -p /tmp/intercept-fs-root
	printf '%s\n' "precreated by intercept-fs/run.sh" >/tmp/intercept-fs-root/intercept-fs.txt
}

intercept_example_run "intercept-fs_qemu-x86_64" seed_intercept_fs
