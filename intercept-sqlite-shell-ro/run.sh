#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
IMAGE_NAME="intercept-sqlite-shell-ro_qemu-x86_64"
DB_PATH="/tmp/intercept-sqlite-ro-root/chinook.db"

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_sqlite_shell_ro()
{
	rm -rf /tmp/intercept-sqlite-ro-root
	mkdir -p /tmp/intercept-sqlite-ro-root
	cp "$REPO_ROOT/sqlite/rootfs/chinook.db" "$DB_PATH"
}

intercept_sqlite_shell_ro_launch_qemu()
{
	qemu-system-x86_64 \
		-nographic \
		-m 8 \
		-cpu max \
		-netdev bridge,id=n0,br=virbr0 \
		-device virtio-net-pci,netdev=n0 \
		-append "$IMAGE_NAME netdev.ip=172.44.0.2/24:172.44.0.1::: env.vars=[ \"HOME=/root\" ] -- $DB_PATH 'SELECT * FROM Album LIMIT 10'" \
		-kernel "workdir/build/$IMAGE_NAME"
}

intercept_example_setup_workdir
intercept_example_require_config
intercept_example_build_guest
seed_intercept_sqlite_shell_ro
intercept_example_require_bridge
intercept_example_prepare_bridge
intercept_sqlite_shell_ro_launch_qemu
