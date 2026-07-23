#!/bin/sh

set -eu

FIXTURE_DIR=/tmp/intercept-bench-micro-root
FIXTURE_PATH="$FIXTURE_DIR/existing.txt"
FIXTURE_SIZE_MIB=1
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

cd "$SCRIPT_DIR"
mkdir -p workdir
mkdir -p "$FIXTURE_DIR"
dd if=/dev/zero of="$FIXTURE_PATH" bs=1M count="$FIXTURE_SIZE_MIB" status=none

cc -O2 -std=gnu17 -Wall -Wextra -pedantic main.c -o workdir/intercept-bench-micro-host
exec ./workdir/intercept-bench-micro-host "$FIXTURE_PATH"
