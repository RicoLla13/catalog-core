#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
FIXTURE_DIR=/tmp/intercept-bench-sqlite-root

cd "$SCRIPT_DIR"
mkdir -p workdir
rm -rf "$FIXTURE_DIR"
mkdir -p "$FIXTURE_DIR"
cp "$REPO_ROOT/sqlite/rootfs/chinook.db" "$FIXTURE_DIR/chinook.db"
cc -O2 -std=gnu17 -Wall -Wextra -pedantic main.c -lsqlite3 -o workdir/intercept-bench-sqlite-host
exec ./workdir/intercept-bench-sqlite-host "$FIXTURE_DIR/chinook.db"
