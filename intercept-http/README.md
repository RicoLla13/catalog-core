# intercept-http

`intercept-http` is the mixed local/remote integration sample.

## Purpose

It proves that one guest can:

- use intercept for a seeded remote preflight path probe
- keep normal local lwIP socket behavior for the HTTP server itself

This sample is not the primary filesystem regression target. Its job is mixed
backend coexistence.

## Host Fixture

`run.sh` seeds:

- `/tmp/intercept-http-root/preflight-ok.txt`

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

4. Query the guest HTTP server from the host:

   ```sh
   curl http://172.44.0.2:8080/
   ```

## Expected Checks

- `access("/tmp/intercept-http-root/preflight-ok.txt", F_OK)` succeeds through intercept
- the guest still binds, listens, accepts, and replies over local sockets
