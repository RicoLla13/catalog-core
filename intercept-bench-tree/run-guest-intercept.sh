#!/bin/sh

set -eu

FIXTURE_DIR=/tmp/intercept-bench-tree-root
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

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
. "$REPO_ROOT/scripts/intercept-example-common.sh"

./apply-config.sh intercept
intercept_example_require_config
intercept_example_build_guest
seed_tree
intercept_example_require_bridge
intercept_example_prepare_bridge

qemu-system-x86_64 \
	-nographic \
	-m 32 \
	-cpu max \
	-netdev bridge,id=n0,br=virbr0 \
	-device virtio-net-pci,netdev=n0 \
	-append "intercept-bench-tree_qemu-x86_64 netdev.ip=172.44.0.2/24:172.44.0.1::: -- $FIXTURE_DIR" \
	-kernel "workdir/build/intercept-bench-tree_qemu-x86_64"

