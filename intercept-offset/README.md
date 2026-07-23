# intercept-offset

`intercept-offset` is the positioned-read sample.

## Purpose

It exists to validate one specific contract:

- `pread64()` must not mutate the fd's current offset

The sample seeds one remote file through normal write flow, then compares
`lseek(..., SEEK_CUR)` before and after `pread()`.

## Host Fixture

`run.sh` prepares:

- `/tmp/intercept-offset-root/`

The guest creates `intercept-offset.txt` during the run.

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

- writes succeed on the tracked remote fd
- `pread()` from multiple offsets succeeds
- the current offset reported by `lseek(..., SEEK_CUR)` is unchanged after `pread()`
