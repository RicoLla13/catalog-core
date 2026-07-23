# intercept-rw

`intercept-rw` is the single-fd sequential I/O sample.

## Purpose

It keeps the regression target small:

- absolute-path open
- repeated writes on one tracked remote file fd
- `lseek()` offset mutations
- `fstat()` on the tracked remote fd
- reads after seek and rewind

## Host Fixture

`run.sh` prepares:

- `/tmp/intercept-rw-root/`

The guest creates `intercept-rw.txt` during the run.

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

- one tracked remote file fd carries the full write/seek/read sequence
- `fstat()` reports the expected size after the writes
