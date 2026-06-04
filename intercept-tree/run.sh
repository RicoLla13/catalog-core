#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_tree()
{
	rm -rf /tmp/intercept-tree-root
	mkdir -p /tmp/intercept-tree-root/pages /tmp/intercept-tree-root/assets
	printf '%s\n%s\n%s\n' \
		"pages/index.txt" \
		"pages/about.txt" \
		"assets/logo.txt" >/tmp/intercept-tree-root/manifest.txt
	printf '%s\n' "index payload for intercept-tree" >/tmp/intercept-tree-root/pages/index.txt
	printf '%s\n' "about payload for intercept-tree" >/tmp/intercept-tree-root/pages/about.txt
	printf '%s\n' "logo payload for intercept-tree" >/tmp/intercept-tree-root/assets/logo.txt
	ln -sfn pages/index.txt /tmp/intercept-tree-root/current-link.txt
	rm -f /tmp/intercept-tree-root/missing.txt
}

intercept_example_run "intercept-tree_qemu-x86_64" seed_intercept_tree
