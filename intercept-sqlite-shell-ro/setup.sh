#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

if [ "${1:-}" = clean ]; then
	intercept_example_clean
	exit 0
fi

check_exists_and_create_symlink()
{
	path="$1"

	if ! test -d workdir/"$path"; then
		if ! test -d ../repos/"$path"; then
			echo "No directory ../repos/$path. Run the top-level setup.sh script first." 1>&2
			exit 1
		fi
		depth=$(echo "$path" | awk -F / '{ print NF }')
		if test "$depth" -eq 1; then
			ln -sfn ../../repos/"$path" workdir/"$path"
		elif test "$depth" -eq 2; then
			ln -sfn ../../../repos/"$path" workdir/"$path"
		else
			echo "Unknown depth of path $path." 1>&2
			exit 1
		fi
	fi
}

if ! test -d workdir; then
	mkdir workdir
fi

if ! test -d workdir/libs; then
	mkdir workdir/libs
fi

check_exists_and_create_symlink "unikraft"
check_exists_and_create_symlink "libs/musl"
check_exists_and_create_symlink "libs/lwip"
check_exists_and_create_symlink "libs/sqlite"

UK_DEFCONFIG="$PWD/configs/qemu.x86_64.defconfig" make defconfig >/dev/null
make olddefconfig >/dev/null
