#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

cd "$SCRIPT_DIR"
. "$REPO_ROOT/scripts/intercept-example-common.sh"

seed_intercept_http()
{
	rm -rf /tmp/intercept-http-root
	mkdir -p /tmp/intercept-http-root
	printf '%s\n' "seeded by intercept-http/run.sh" >/tmp/intercept-http-root/preflight-ok.txt
}

intercept_example_run "intercept-http_qemu-x86_64" seed_intercept_http
