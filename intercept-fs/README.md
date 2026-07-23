# intercept-fs

`intercept-fs` is the broader relative-filesystem flow sample.

## Purpose

It combines:

- dirfd acquisition and classification
- host-seeded relative metadata lookup
- relative file creation through a tracked remote dirfd
- repeated writes, reopen, `fstat()`, `read()`, and `lseek()`
- local `ENOTDIR` rejection on a tracked remote regular file

## Host Fixture

`run.sh` seeds:

- `/tmp/intercept-fs-root/intercept-fs.txt`

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

- the initial seeded file is visible through relative `fstatat()`
- relative create/write/reopen/read works through one tracked remote dirfd
- path-based and fd-based metadata agree
- `fstatat(filefd, "child", ...)` fails with `ENOTDIR`
