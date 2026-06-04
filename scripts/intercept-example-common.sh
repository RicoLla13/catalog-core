#!/bin/sh

set -eu

intercept_example_check_exists_and_create_symlink()
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

intercept_example_setup_workdir()
{
	if ! test -d workdir; then
		mkdir workdir
	fi

	intercept_example_check_exists_and_create_symlink "unikraft"

	if ! test -d workdir/libs; then
		mkdir workdir/libs
	fi

	intercept_example_check_exists_and_create_symlink "libs/musl"
	intercept_example_check_exists_and_create_symlink "libs/lwip"
}

intercept_example_require_config()
{
	if [ ! -f .config ]; then
		echo "No .config found. Run 'make menuconfig' first and select x86_64 with QEMU/KVM." >&2
		exit 1
	fi
}

intercept_example_build_guest()
{
	make -j"$(nproc)" CFLAGS="-std=gnu17" EXTRA_CFLAGS="-std=gnu17"
}

intercept_example_require_bridge()
{
	if [ ! -f /etc/qemu/bridge.conf ] || ! grep -Eq '^[[:space:]]*allow[[:space:]]+all([[:space:]]|$)' /etc/qemu/bridge.conf; then
		echo "/etc/qemu/bridge.conf must exist and allow bridge networking (for example: 'allow all')." >&2
		exit 1
	fi
}

intercept_example_prepare_bridge()
{
	if ! ip link show virbr0 >/dev/null 2>&1; then
		sudo ip link add dev virbr0 type bridge
	fi

	if ! ip addr show dev virbr0 | grep -q '172\.44\.0\.1/24'; then
		sudo ip address add 172.44.0.1/24 dev virbr0
	fi

	sudo ip link set dev virbr0 up
}

intercept_example_launch_qemu()
{
	image_name="$1"

	qemu-system-x86_64 \
		-nographic \
		-m 8 \
		-cpu max \
		-netdev bridge,id=n0,br=virbr0 \
		-device virtio-net-pci,netdev=n0 \
		-append "$image_name netdev.ip=172.44.0.2/24:172.44.0.1::: -- " \
		-kernel "workdir/build/$image_name"
}

intercept_example_run()
{
	image_name="$1"
	seed_fn="$2"

	intercept_example_setup_workdir
	intercept_example_require_config
	intercept_example_build_guest
	"$seed_fn"
	intercept_example_require_bridge
	intercept_example_prepare_bridge
	intercept_example_launch_qemu "$image_name"
}
