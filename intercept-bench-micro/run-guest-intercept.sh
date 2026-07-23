#!/bin/sh

set -eu

FIXTURE_DIR=/tmp/intercept-bench-micro-root
FIXTURE_PATH="$FIXTURE_DIR/existing.txt"
FIXTURE_SIZE_MIB=1
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

./apply-config.sh intercept

reset_remote_fixture()
{
	rm -rf "$FIXTURE_DIR"
	mkdir -p "$FIXTURE_DIR"
	dd if=/dev/zero of="$FIXTURE_PATH" bs=1M count="$FIXTURE_SIZE_MIB" status=none
}

intercept_example_require_config
intercept_example_build_guest
reset_remote_fixture
intercept_example_require_bridge
intercept_example_prepare_bridge

qemu-system-x86_64 \
	-nographic \
	-m 32 \
	-cpu max \
	-netdev bridge,id=n0,br=virbr0 \
	-device virtio-net-pci,netdev=n0 \
	-append "intercept-bench-micro_qemu-x86_64 netdev.ip=172.44.0.2/24:172.44.0.1::: -- $FIXTURE_PATH" \
	-kernel "workdir/build/intercept-bench-micro_qemu-x86_64"
