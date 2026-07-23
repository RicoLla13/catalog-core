#!/bin/sh

set -eu

GUEST_ROOT=/tmp/intercept-bench-sqlite-root
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"
./apply-config.sh local
intercept_example_require_config
intercept_example_build_guest
rm -rf 9pfs-rootfs
mkdir -p 9pfs-rootfs/tmp/intercept-bench-sqlite-root
cp "$REPO_ROOT/sqlite/rootfs/chinook.db" 9pfs-rootfs/tmp/intercept-bench-sqlite-root/chinook.db

qemu-system-x86_64 -nographic -m 32 -cpu max \
	-kernel workdir/build/intercept-bench-sqlite_qemu-x86_64 \
	-append "intercept-bench-sqlite_qemu-x86_64 vfs.fstab=[ \"fs0:/:9pfs:::\" ] -- $GUEST_ROOT/chinook.db" \
	-fsdev local,id=myid,path=$(pwd)/9pfs-rootfs/,security_model=none \
	-device virtio-9p-pci,fsdev=myid,mount_tag=fs0
