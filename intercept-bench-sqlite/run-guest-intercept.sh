#!/bin/sh

set -eu

FIXTURE_DIR=/tmp/intercept-bench-sqlite-root
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"
./apply-config.sh intercept
intercept_example_require_config
intercept_example_build_guest
rm -rf "$FIXTURE_DIR"
mkdir -p "$FIXTURE_DIR"
cp "$REPO_ROOT/sqlite/rootfs/chinook.db" "$FIXTURE_DIR/chinook.db"
intercept_example_require_bridge
intercept_example_prepare_bridge

qemu-system-x86_64 -nographic -m 32 -cpu max \
	-netdev bridge,id=n0,br=virbr0 -device virtio-net-pci,netdev=n0 \
	-append "intercept-bench-sqlite_qemu-x86_64 netdev.ip=172.44.0.2/24:172.44.0.1::: -- $FIXTURE_DIR/chinook.db" \
	-kernel workdir/build/intercept-bench-sqlite_qemu-x86_64
