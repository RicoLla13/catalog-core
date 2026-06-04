# intercept-fs

`intercept-fs` is the main remote-filesystem sample for the current intercept
layer. It uses the host syscall server to access `/tmp`, opens a remote
directory fd, creates a file with relative `openat()`, writes data, queries
metadata with `fstatat()` and `fstat()`, then reads the file back.

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

## What `setup.sh` Does

`setup.sh` prepares `workdir/` and creates symlinks to:

- `workdir/unikraft`
- `workdir/libs/musl`
- `workdir/libs/lwip`

`run.sh` invokes `setup.sh` again before building so the sample can be rerun
after a clean without extra manual steps.

## What `run.sh` Does

`run.sh` uses the same QEMU flow for each intercept sample:

1. runs `./setup.sh`
2. builds the guest image
3. seeds the host fixture file `/tmp/intercept-fs.txt` for the initial
   relative `fstatat()` probe
4. checks that `/etc/qemu/bridge.conf` already allows bridge networking
5. reuses or creates `virbr0`
6. ensures `virbr0` carries `172.44.0.1/24`
7. brings the bridge up
8. starts `qemu-system-x86_64` with the guest image and a bridged virtio NIC

`run.sh` does not modify `/etc/qemu/bridge.conf`; it only verifies the host
bridge prerequisite before launching QEMU.

The launched image is `workdir/build/intercept-fs_qemu-x86_64`.

## Network Layout

The sample uses one bridged network for both guest networking and intercept
RPC:

- host bridge: `virbr0`
- host IP: `172.44.0.1`
- guest IP: `172.44.0.2`
- intercept RPC server: `172.44.0.1:9999`

The guest lwIP stack connects to the host syscall server over TCP. Every
intercepted filesystem syscall is encoded as ONC RPC/XDR and sent through that
connection.

## Current Flow

The sample currently exercises:

1. `access("/tmp", F_OK)`
2. `openat(AT_FDCWD, "/tmp", O_RDONLY | O_DIRECTORY, 0)`
3. `openat(AT_FDCWD, "/tmp", O_RDONLY, 0)`
4. `fstatat(non_o_directory_dirfd, "intercept-fs.txt", &st, 0)` on a
   host-seeded file
5. `close(non_o_directory_dirfd)`
6. `openat(dirfd, "intercept-fs.txt", O_CREAT | O_TRUNC | O_WRONLY, 0644)`
7. two `write()` calls on the remote file
8. `close(fd)`
9. `fstatat(dirfd, "intercept-fs.txt", &st, 0)`
10. `openat(dirfd, "intercept-fs.txt", O_RDONLY, 0)`
11. `fstatat(filefd, "intercept-fs-missing-child", &st, 0)` -> `ENOTDIR`
12. `lseek(fd, 8, SEEK_SET)`
13. `fstat(fd, &st)`
14. `read(fd, ...)`
15. `lseek(fd, 0, SEEK_SET)`
16. `read(fd, ...)`
17. `close(fd)`
18. `close(dirfd)`

This gives you one path-based probe, one tracked remote directory fd, one
relative `openat()` flow, and both path-based and fd-based metadata checks.

## Expected Guest Output

The exact remote fd values vary, but a successful run should look like:

```text
access("/tmp", F_OK) succeeded: rc=0 errno=0
openat(AT_FDCWD, "/tmp", O_RDONLY|O_DIRECTORY) succeeded: fd=<remote-dirfd> errno=0
openat(AT_FDCWD, "/tmp", O_RDONLY) succeeded: fd=<remote-dirfd> errno=0
fstatat(<remote-dirfd>, "intercept-fs.txt", 0) via non-O_DIRECTORY dirfd succeeded: rc=0 errno=0 mode=<mode> size=<seed-size> nlink=<nlink>
close(<remote-dirfd>) succeeded: rc=0 errno=0
openat(<remote-dirfd>, "intercept-fs.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644) succeeded: fd=<remote-fd> errno=0
write(<remote-fd>, <n>) succeeded: rc=<n> errno=0
write(<remote-fd>, <n>) succeeded: rc=<n> errno=0
close(<remote-fd>) succeeded: rc=0 errno=0
fstatat(<remote-dirfd>, "intercept-fs.txt", 0) succeeded: rc=0 errno=0 mode=<mode> size=<nbytes> nlink=<nlink>
openat(<remote-dirfd>, "intercept-fs.txt", O_RDONLY) succeeded: fd=<remote-fd> errno=0
fstatat(<remote-fd>, "intercept-fs-missing-child", 0) on regular-file dirfd correctly failed: rc=-1 errno=20 (Not a directory)
lseek(<remote-fd>, 8, SEEK_SET) succeeded: off=8 errno=0
fstat(<remote-fd>) succeeded: rc=0 errno=0 mode=<mode> size=<nbytes> nlink=<nlink>
read(<remote-fd>, 159) succeeded: rc=<nbytes> errno=0 data=" from intercept-fs using relative openat"
lseek(<remote-fd>, 0, SEEK_SET) succeeded: off=0 errno=0
read(<remote-fd>, 159) succeeded: rc=<nbytes> errno=0 data="written from intercept-fs using relative openat"
close(<remote-fd>) succeeded: rc=0 errno=0
close(<remote-dirfd>) succeeded: rc=0 errno=0
```

## Current Limits

The sample intentionally stays inside the currently implemented syscall subset.
It does not try to clean up the remote file because `unlink()` is not
intercepted yet.
