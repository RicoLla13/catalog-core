#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR"
rm -rf workdir 9pfs-rootfs results
rm -f .config .config.old
rm -rf /tmp/intercept-bench-sqlite-root
