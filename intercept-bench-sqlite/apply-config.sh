#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
MODE=${1:-}
BASE_DEFCONFIG="$SCRIPT_DIR/configs/qemu.x86_64.base.defconfig"
CONFIG_STAMP="$SCRIPT_DIR/workdir/.last-config-mode"

case "$MODE" in
	local|intercept) FRAGMENT="$SCRIPT_DIR/configs/qemu.x86_64.$MODE.fragment" ;;
	*) echo "usage: $0 {local|intercept}" >&2; exit 1 ;;
esac

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"
intercept_example_setup_workdir

if [ -f .config ] && [ -f "$CONFIG_STAMP" ] &&
	[ "$(cat "$CONFIG_STAMP")" = "$MODE" ] &&
	[ .config -nt "$BASE_DEFCONFIG" ] && [ .config -nt "$FRAGMENT" ]; then
	exit 0
fi

make properclean >/dev/null
UK_DEFCONFIG="$BASE_DEFCONFIG" make defconfig >/dev/null
KCONFIG_CONFIG=.config ./workdir/unikraft/support/kconfig/merge_config.sh -m .config "$FRAGMENT" >/dev/null
make olddefconfig >/dev/null
printf '%s\n' "$MODE" > "$CONFIG_STAMP"
