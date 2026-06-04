# intercept-tree

`intercept-tree` is the complex pre-SQLite read-only filesystem sample.

## Purpose

It is the main “application-like” readiness sample before SQLite. It validates:

- path metadata on a seeded directory tree
- `open()` on absolute paths
- `openat()` on relative paths through a tracked remote dirfd
- `fstat()`, `read()`, and one `pread()` check during tree traversal
- symlink handling through `stat()` and `lstat()`
- negative `ENOENT` and `ENOTDIR` cases

## Host Fixture

`run.sh` seeds:

- `/tmp/intercept-tree-root/manifest.txt`
- `/tmp/intercept-tree-root/pages/index.txt`
- `/tmp/intercept-tree-root/pages/about.txt`
- `/tmp/intercept-tree-root/assets/logo.txt`
- `/tmp/intercept-tree-root/current-link.txt -> pages/index.txt`

and removes:

- `/tmp/intercept-tree-root/missing.txt`

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

- the guest reads the seeded manifest and visits each listed file
- one file is read via `pread()` without offset mutation
- the other files are opened relative to the tracked remote root dirfd
- `stat(link)` follows the target while `lstat(link)` reports a symlink
- `stat(missing)` fails with `ENOENT`
- `fstatat(filefd, "child", ...)` fails with `ENOTDIR`
