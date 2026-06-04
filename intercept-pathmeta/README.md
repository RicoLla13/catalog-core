# intercept-pathmeta

`intercept-pathmeta` is the focused path-metadata sample added for the new
`open()`, `stat()`, and `lstat()` hooks.

## Purpose

It owns the path-based metadata contract:

- `open()` on absolute paths
- `stat()` on regular files and missing paths
- `lstat()` on regular files and symlinks
- one `fstat()` cross-check against path metadata

## Host Fixture

`run.sh` seeds:

- `/tmp/intercept-pathmeta-root/existing.txt`
- `/tmp/intercept-pathmeta-root/existing-link.txt -> existing.txt`

and removes:

- `/tmp/intercept-pathmeta-root/runtime.txt`
- `/tmp/intercept-pathmeta-root/missing.txt`

## Run

1. Start the host syscall server:

   ```sh
   cd ../repos/syscall-server
   make create_folders server client test_suite
   ./build/syscall_server
   ```

2. Configure the guest once:

   ```sh
   ./setup.sh
   make menuconfig
   ```

3. Launch the sample:

   ```sh
   ./run.sh
   ```

## Expected Checks

- `stat(existing)` and `lstat(existing)` both report a regular file
- `stat(link)` follows the target while `lstat(link)` reports a symlink
- `open(runtime)` create/write flow succeeds
- `stat(runtime)` and `fstat(fd)` agree on size
- `stat(missing)` fails with `ENOENT`
