# intercept-simple

`intercept-simple` is the smallest syscall-intercept example in this tree. It
boots a Unikraft guest, reaches the host syscall server over the intercept TCP
transport, and exercises `access()` against host paths.

## Run Order

1. Prepare the sample workdir:

   ```sh
   ./setup.sh
   ```

2. Configure the sample for `x86_64` and `QEMU/KVM`:

   ```sh
   make menuconfig
   ```

   Keep `LIBINTERCEPT`, `LIBLWIP`, and the socket transport enabled. The sample
   `Config.uk` selects the required defaults, but you still need to choose the
   target architecture and platform.

   `run.sh` expects the resulting `.config` to exist. There is no repo-local
   `defconfig` for this sample yet.

3. Build the host syscall server in a separate shell:

   ```sh
   cd ../repos/syscall-server
   make create_folders server client test_suite
   ./build/syscall_server
   ```

4. Start the guest:

   ```sh
   ./run.sh
   ```

## What `setup.sh` Does

`setup.sh` creates `workdir/` and links the shared repositories from
`../repos/`:

- `workdir/unikraft`
- `workdir/libs/musl`
- `workdir/libs/lwip`

Run it once before the first configuration. `run.sh` calls it again so repeated
runs stay safe after cleaning the workdir.

## What `run.sh` Does

`run.sh` performs the whole local launch flow:

1. runs `./setup.sh`
2. builds the guest with `make -j"$(nproc)" CFLAGS="-std=gnu17"`
3. checks that `/etc/qemu/bridge.conf` already allows bridge networking
4. creates `virbr0` if it is missing
5. assigns `172.44.0.1/24` to `virbr0` if that address is missing
6. brings `virbr0` up
7. starts `qemu-system-x86_64` with a bridged virtio NIC and the guest image

`run.sh` does not rewrite `/etc/qemu/bridge.conf`; it expects that host
prerequisite to be configured already.

The script only supports `x86_64` on QEMU.

## Network Layout

The intercept transport uses the same bridged QEMU topology across the current
intercept samples:

- host bridge: `virbr0`
- host bridge address: `172.44.0.1/24`
- guest address: `172.44.0.2/24`
- host syscall server endpoint: `172.44.0.1:9999`

Traffic flow:

- the guest lwIP stack owns `172.44.0.2`
- intercepted syscalls open a TCP connection from the guest to
  `172.44.0.1:9999`
- the host syscall server answers ONC RPC requests on that socket

## Expected Guest Output

The guest should show one successful host-path probe and one missing-path
probe:

```text
[] access("/tmp", F_OK) succeeded: rc=0 errno=0
[] access("/tmp/intercept-simple-missing", F_OK) missing as expected: rc=-1 errno=2 (No such file or directory)
```
