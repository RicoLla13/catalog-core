# intercept-simple

`intercept-simple` is the smallest remote-path smoke test in this tree.

## Purpose

It proves only that:

- the guest reaches the host syscall server
- a path-based intercept call succeeds on an existing host fixture
- remote `ENOENT` is surfaced correctly for a missing host path

## Host Fixture

`run.sh` seeds:

- `/tmp/intercept-simple-root/existing.txt`

and removes:

- `/tmp/intercept-simple-root/missing.txt`

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

- `access("/tmp/intercept-simple-root/existing.txt", F_OK)` succeeds
- `access("/tmp/intercept-simple-root/missing.txt", F_OK)` fails with `ENOENT`
