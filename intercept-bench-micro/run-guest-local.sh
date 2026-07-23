#!/bin/sh

set -eu

FIXTURE_NAME=existing.txt
FIXTURE_SIZE_MIB=1
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

./apply-config.sh local
intercept_example_require_config
intercept_example_build_guest

rm -rf 9pfs-rootfs
mkdir -p 9pfs-rootfs
dd if=/dev/zero of=9pfs-rootfs/"$FIXTURE_NAME" bs=1M count="$FIXTURE_SIZE_MIB" status=none

qemu-system-x86_64 \
	-nographic \
	-m 32 \
	-cpu max \
	-kernel "workdir/build/intercept-bench-micro_qemu-x86_64" \
	-append "intercept-bench-micro_qemu-x86_64 vfs.fstab=[ \"fs0:/:9pfs:::\" ] -- /$FIXTURE_NAME" \
	-fsdev local,id=myid,path=$(pwd)/9pfs-rootfs/,security_model=none \
	-device virtio-9p-pci,fsdev=myid,mount_tag=fs0
