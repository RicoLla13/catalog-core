# intercept-probe

`intercept-probe` is the path-only smoke test for the guest intercept layer.
It proves that the guest can reach the host syscall server and round-trip
simple `access()` checks without allocating any tracked remote file
descriptors.

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

This sample intentionally covers only:

1. `access("/tmp", F_OK)` for an existing host path
2. `access("/tmp/intercept-probe-missing", F_OK)` for a missing host path

If this sample fails, the problem is likely transport, RPC envelope handling,
or the basic path-based intercept flow rather than tracked remote fd behavior.
