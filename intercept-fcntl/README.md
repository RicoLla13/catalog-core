# intercept-fcntl

`intercept-fcntl` is the focused fd-control sample.

## Purpose

It validates the currently supported `fcntl()` subset on one tracked remote
file descriptor:

- `F_GETFD` / `F_SETFD`
- `F_GETFL` / `F_SETFL`
- `F_DUPFD`
- `F_SETLK` unlock flow

## Host Fixture

`run.sh` prepares:

- `/tmp/intercept-fcntl-root/`

The guest creates `intercept-fcntl.txt` during the run.

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

- descriptor-local flags behave correctly on the guest-visible fd
- file-status flags survive `F_SETFL`
- `F_DUPFD` allocates a second guest-visible remote fd
- lock and unlock requests round-trip through the server
