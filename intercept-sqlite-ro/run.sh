#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_sqlite_ro()
{
	rm -rf /tmp/intercept-sqlite-ro-root
	mkdir -p /tmp/intercept-sqlite-ro-root
	cp "$REPO_ROOT/sqlite/rootfs/chinook.db" /tmp/intercept-sqlite-ro-root/chinook.db
}

intercept_example_run "intercept-sqlite-ro_qemu-x86_64" seed_intercept_sqlite_ro
