#!/bin/sh

make -j"$(nproc)" CFLAGS="-std=gnu17" EXTRA_CFLAGS="-std=gnu17"
qemu-system-x86_64 -nographic -m 8 -cpu max -kernel workdir/build/c-hello_qemu-x86_64
