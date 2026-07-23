# intercept-dirfd

`intercept-dirfd` is the focused dirfd-policy sample.

## Purpose

It isolates:

- remote directory classification after `openat()`
- relative path resolution through tracked remote dirfds
- local `ENOTDIR` rejection when a tracked remote regular file is reused as a dirfd

## Host Fixture

`run.sh` prepares:

- `/tmp/intercept-dirfd-root/`

The sample itself creates `intercept-dirfd.txt` inside that directory.

## Run

1. Start the host syscall server:

   ```sh
   cd ../repos/syscall-server
   make create_folders server client test_suite
   ./build/syscall_server
   ```

2. Configure the guest:

   ```sh
   ./setup.sh clean
   ./setup.sh
   ```

3. Launch the sample:

   ```sh
   ./run.sh
   ```

## Expected Checks

- both `openat(..., O_DIRECTORY)` and `openat(..., O_RDONLY)` on the seeded root behave as remote directories
- relative `openat()` and `fstatat()` through tracked dirfds succeed
- `fstatat(filefd, "child", ...)` fails with `ENOTDIR`
