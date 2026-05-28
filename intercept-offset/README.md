# intercept-offset

`intercept-offset` is the offset-stability sample for tracked remote file
descriptors. It exists to validate `pread64()` separately from the sequential
`read()` and `lseek()` flow in `intercept-rw`.

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

1. create/truncate and write on one tracked remote file fd
2. baseline `SEEK_CUR` position capture
3. `pread64()` from multiple offsets
4. `SEEK_CUR` position check after `pread64()` to prove the current offset did
   not move
