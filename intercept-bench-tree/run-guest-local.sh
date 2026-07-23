#!/bin/sh

set -eu

GUEST_ROOT=/tmp/intercept-bench-tree-root
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

seed_tree()
{
	rm -rf 9pfs-rootfs
	mkdir -p 9pfs-rootfs/tmp/intercept-bench-tree-root/pages \
		9pfs-rootfs/tmp/intercept-bench-tree-root/assets
	root=9pfs-rootfs/tmp/intercept-bench-tree-root
	printf '%s\n%s\n%s\n' \
		"pages/index.txt" \
		"pages/about.txt" \
		"assets/logo.txt" >"$root/manifest.txt"
	printf '%s\n' "index payload for intercept-bench-tree" >"$root/pages/index.txt"
	printf '%s\n' "about payload for intercept-bench-tree" >"$root/pages/about.txt"
	printf '%s\n' "logo payload for intercept-bench-tree" >"$root/assets/logo.txt"
	ln -sfn pages/index.txt "$root/current-link.txt"
	rm -f "$root/missing.txt"
}

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

./apply-config.sh local
intercept_example_require_config
intercept_example_build_guest
seed_tree

qemu-system-x86_64 \
	-nographic \
	-m 32 \
	-cpu max \
	-kernel "workdir/build/intercept-bench-tree_qemu-x86_64" \
	-append "intercept-bench-tree_qemu-x86_64 vfs.fstab=[ \"fs0:/:9pfs:::\" ] -- $GUEST_ROOT" \
	-fsdev local,id=myid,path=$(pwd)/9pfs-rootfs/,security_model=none \
	-device virtio-9p-pci,fsdev=myid,mount_tag=fs0

