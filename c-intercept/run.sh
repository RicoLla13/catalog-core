#!/bin/sh

set -eu

make -j"$(nproc)" CFLAGS="-std=gnu17" EXTRA_CFLAGS="-std=gnu17"

sudo mkdir -p /etc/qemu
printf '%s\n' "allow all" | sudo tee /etc/qemu/bridge.conf >/dev/null

if ! ip link show virbr0 >/dev/null 2>&1; then
    sudo ip link add dev virbr0 type bridge
fi

if ! ip addr show dev virbr0 | grep -q '172\.44\.0\.1/24'; then
    sudo ip address add 172.44.0.1/24 dev virbr0
fi

sudo ip link set dev virbr0 up

sudo qemu-system-x86_64 \
    -nographic \
    -m 8 \
    -cpu max \
    -netdev bridge,id=n0,br=virbr0 \
    -device virtio-net-pci,netdev=n0 \
    -append "netdev.ip=172.44.0.2/24:172.44.0.1::: --" \
    -kernel workdir/build/c-intercept_qemu-x86_64
