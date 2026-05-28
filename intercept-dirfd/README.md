# intercept-dirfd

`intercept-dirfd` is the remote dirfd policy sample. It exists to test
descriptor classification and relative path behavior without mixing in broader
read/write coverage.

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

1. strict directory open with `O_DIRECTORY`
2. directory open without `O_DIRECTORY`
3. relative `openat()` through the classified directory fd
4. relative `fstatat()` through both directory-fd forms
5. local `ENOTDIR` rejection when a tracked remote regular file is misused as a dirfd

If this sample fails while `intercept-probe` passes, the likely problem is in
tracked remote fd classification or dirfd policy rather than transport alone.
