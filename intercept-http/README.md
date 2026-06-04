# intercept-http

`intercept-http` is the HTTP-server-facing intercept sample. It stays close to
`c-http`: the server still uses normal guest sockets through lwIP, but it also
runs one intercept preflight `access("/tmp", F_OK)` during startup so the same
guest demonstrates both paths:

- remote host filesystem access through intercept RPC
- local guest TCP sockets for HTTP traffic

## Run Order

1. Prepare the sample workdir:

   ```sh
   ./setup.sh
   ```

2. Configure the sample for `x86_64` and `QEMU/KVM`:

   ```sh
   make menuconfig
   ```

   `run.sh` expects the resulting `.config` to exist. There is no repo-local
   `defconfig` for this sample yet.

3. Start the host syscall server in a separate shell:

   ```sh
   cd ../repos/syscall-server
   make create_folders server client test_suite
   ./build/syscall_server
   ```

4. Launch the guest:

   ```sh
   ./run.sh
   ```

5. From the host, query the guest HTTP server:

   ```sh
   curl http://172.44.0.2:8080/
   ```

## What `setup.sh` Does

`setup.sh` creates `workdir/` and links:

- `workdir/unikraft`
- `workdir/libs/musl`
- `workdir/libs/lwip`

## What `run.sh` Does

`run.sh`:

1. runs `./setup.sh`
2. builds `workdir/build/intercept-http_qemu-x86_64`
3. checks that `/etc/qemu/bridge.conf` already allows bridge networking
4. creates or reuses `virbr0`
5. ensures the host bridge address is `172.44.0.1/24`
6. launches QEMU with a bridged virtio NIC
7. passes `netdev.ip=172.44.0.2/24:172.44.0.1:::` to the guest

`run.sh` does not modify `/etc/qemu/bridge.conf`; it fails early if the host
bridge prerequisite is missing.

The script is intentionally limited to `x86_64` on QEMU.

## Network Layout

The same bridge carries both intercept RPC traffic and HTTP traffic:

- host bridge: `virbr0`
- host bridge IP: `172.44.0.1`
- guest IP: `172.44.0.2`
- host syscall server: `172.44.0.1:9999`
- guest HTTP server: `172.44.0.2:8080`

There are two distinct flows:

- guest `access("/tmp", F_OK)` goes from the guest to the host syscall server
  over the intercept TCP transport
- host `curl http://172.44.0.2:8080/` talks to the guest HTTP server over the
  same bridge, but through the normal lwIP socket stack

## Current Behavior

At boot the sample:

1. probes the host filesystem with `access("/tmp", F_OK)`
2. binds a TCP socket on port `8080`
3. accepts connections
4. returns a fixed HTTP response body

This means the sample already proves that the current hook strategy can mix:

- intercepted remote file syscalls
- non-intercepted local socket syscalls

inside the same guest.

## What Is Still Missing for Remote File Serving

`intercept-http` is intentionally conservative today. It does not yet open or
serve files from the remote host filesystem. To move from a fixed reply to
remote file serving, Unikraft-side intercept work still needs:

- `lseek()`
- `pread64()`
- `writev()`
- `fcntl()`
- a richer remote-fd metadata table instead of the current bitmap-only model

Those details are documented under
[gen-docs/examples/INTERCEPT_HTTP.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/examples/INTERCEPT_HTTP.md)
and
[gen-docs/unikraft-intercept/HTTP_SERVER_ROADMAP.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/HTTP_SERVER_ROADMAP.md).
