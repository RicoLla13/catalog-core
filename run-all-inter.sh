#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SERVER_DIR="$SCRIPT_DIR/repos/syscall-server"
SERVER_BIN="$SERVER_DIR/build/syscall_server"
LOGS_DIR="$SCRIPT_DIR/logs"
HTTP_TIMEOUT="${INTERCEPT_HTTP_TIMEOUT:-15}"

EXAMPLES="
intercept-probe
intercept-dirfd
intercept-rw
intercept-offset
intercept-http
"

server_pid=""
server_log=""

cleanup_server() {
	if [ -n "${server_pid}" ] && kill -0 "${server_pid}" 2>/dev/null; then
		kill "${server_pid}" 2>/dev/null || true
		wait "${server_pid}" 2>/dev/null || true
	fi
	server_pid=""
}

setup_bridge() {
	sudo mkdir -p /etc/qemu
	printf '%s\n' "allow all" | sudo tee /etc/qemu/bridge.conf >/dev/null

	if ! ip link show virbr0 >/dev/null 2>&1; then
		sudo ip link add dev virbr0 type bridge
	fi

	if ! ip addr show dev virbr0 | grep -q '172\.44\.0\.1/24'; then
		sudo ip address add 172.44.0.1/24 dev virbr0
	fi

	sudo ip link set dev virbr0 up
}

build_example() {
	example="$1"
	build_log="$(mktemp)"

	if [ ! -f "$SCRIPT_DIR/$example/.config" ]; then
		echo "[x] $example has no .config; run make menuconfig first" >&2
		rm -f "$build_log"
		return 1
	fi

	if ! (
		cd "$SCRIPT_DIR/$example"
		./setup.sh
		make -j"$(nproc)" CFLAGS="-std=gnu17" EXTRA_CFLAGS="-std=gnu17"
	) >"$build_log" 2>&1; then
		echo "[x] build failed for $example" >&2
		cat "$build_log" >&2
		rm -f "$build_log"
		return 1
	fi

	rm -f "$build_log"
}

start_server() {
	if [ ! -x "$SERVER_BIN" ]; then
		echo "[x] missing syscall server binary at $SERVER_BIN" >&2
		echo "[x] build it first with: cd repos/syscall-server && make create_folders server client test_suite" >&2
		exit 1
	fi

	(
		cd "$SERVER_DIR"
		exec ./build/syscall_server
	) >"$server_log" 2>&1 &
	server_pid=$!
	sleep 1
	if ! kill -0 "${server_pid}" 2>/dev/null; then
		echo "[x] syscall server failed to start" >&2
		wait "${server_pid}" || true
		server_pid=""
		exit 1
	fi
}

run_vm() {
	example="$1"
	example_log="$2"
	kernel="$SCRIPT_DIR/$example/workdir/build/${example}_qemu-x86_64"

	if [ ! -f "$kernel" ]; then
		echo "[x] missing kernel for $example at $kernel" >&2
		return 1
	fi

	if [ "$example" = "intercept-http" ]; then
		if ! command -v timeout >/dev/null 2>&1; then
			echo "[x] timeout command not found; cannot run intercept-http non-interactively" >&2
			return 1
		fi

		set +e
		sudo timeout "${HTTP_TIMEOUT}" qemu-system-x86_64 \
			-nographic \
			-m 8 \
			-cpu max \
			-netdev bridge,id=n0,br=virbr0 \
			-device virtio-net-pci,netdev=n0 \
			-append "${example}_qemu-x86_64 netdev.ip=172.44.0.2/24:172.44.0.1::: -- " \
			-kernel "$kernel" >"$example_log" 2>&1
		rc=$?
		set -e
		if [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ]; then
			return "$rc"
		fi
		return 0
	fi

	sudo qemu-system-x86_64 \
		-nographic \
		-m 8 \
		-cpu max \
		-netdev bridge,id=n0,br=virbr0 \
		-device virtio-net-pci,netdev=n0 \
		-append "${example}_qemu-x86_64 netdev.ip=172.44.0.2/24:172.44.0.1::: -- " \
		-kernel "$kernel" >"$example_log" 2>&1
}

run_example() {
	example="$1"
	example_dir="$LOGS_DIR/$example"
	example_log="$example_dir/${example}-logs.txt"

	echo "[] starting $example"
	mkdir -p "$example_dir"
	server_log="$example_dir/server-logs.txt"

	build_example "$example"
	start_server
	run_vm "$example" "$example_log"
	cleanup_server

	echo "[] finished $example"
	echo "[] logs: $example_dir"
}

trap cleanup_server EXIT INT TERM

rm -rf "$LOGS_DIR"
mkdir -p "$LOGS_DIR"
setup_bridge

for example in $EXAMPLES; do
	run_example "$example"
done
