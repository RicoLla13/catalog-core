# intercept-rw

`intercept-rw` is the sequential remote file I/O sample. It keeps the focus on
one tracked remote file descriptor and the syscalls that currently operate on
that fd: `openat()`, `write()`, `lseek()`, `fstat()`, `read()`, and `close()`.

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

1. create/truncate through absolute-path `openat()`
2. repeated `write()` calls on one tracked remote file fd
3. offset checks through `lseek()`
4. `fstat()` on the tracked remote fd
5. reads after seek and after rewind

If this sample fails while `intercept-probe` and `intercept-dirfd` pass, the
likely problem is in tracked remote file I/O rather than dirfd classification
or transport setup.
