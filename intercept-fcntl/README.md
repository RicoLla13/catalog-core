# intercept-fcntl

`intercept-fcntl` is the focused remote fd control sample. It validates the
guest-side `fcntl()` intercept path on one tracked remote file descriptor.

## Run Order

1. Prepare the sample workdir:

   ```sh
   ./setup.sh
   ```

2. Configure the sample for `x86_64` and `QEMU/KVM`:

   ```sh
   make menuconfig
   ```

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

## Contract

This sample intentionally covers:

1. descriptor-local `F_GETFD` / `F_SETFD`
2. remote `F_GETFL` / `F_SETFL`
3. remote `F_DUPFD`
4. remote `F_SETLK` and unlock through `F_SETLK`

If this sample fails while `intercept-rw` passes, the likely problem is in
remote fd-control behavior rather than in basic sequential I/O.
