#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_pathmeta()
{
	rm -rf /tmp/intercept-pathmeta-root
	mkdir -p /tmp/intercept-pathmeta-root
	printf '%s\n' "seeded pathmeta fixture" >/tmp/intercept-pathmeta-root/existing.txt
	ln -sfn existing.txt /tmp/intercept-pathmeta-root/existing-link.txt
	rm -f /tmp/intercept-pathmeta-root/runtime.txt
	rm -f /tmp/intercept-pathmeta-root/missing.txt
}

intercept_example_run "intercept-pathmeta_qemu-x86_64" seed_intercept_pathmeta
