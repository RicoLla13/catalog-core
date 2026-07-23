#!/bin/sh

set -eu

FIXTURE_DIR=/tmp/intercept-bench-tree-root
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

seed_tree()
{
	rm -rf "$FIXTURE_DIR"
	mkdir -p "$FIXTURE_DIR/pages" "$FIXTURE_DIR/assets"
	printf '%s\n%s\n%s\n' \
		"pages/index.txt" \
		"pages/about.txt" \
		"assets/logo.txt" >"$FIXTURE_DIR/manifest.txt"
	printf '%s\n' "index payload for intercept-bench-tree" >"$FIXTURE_DIR/pages/index.txt"
	printf '%s\n' "about payload for intercept-bench-tree" >"$FIXTURE_DIR/pages/about.txt"
	printf '%s\n' "logo payload for intercept-bench-tree" >"$FIXTURE_DIR/assets/logo.txt"
	ln -sfn pages/index.txt "$FIXTURE_DIR/current-link.txt"
	rm -f "$FIXTURE_DIR/missing.txt"
}

cd "$SCRIPT_DIR"
mkdir -p workdir
seed_tree
cc -O2 -std=gnu17 -Wall -Wextra -pedantic main.c -o workdir/intercept-bench-tree-host
exec ./workdir/intercept-bench-tree-host "$FIXTURE_DIR"

