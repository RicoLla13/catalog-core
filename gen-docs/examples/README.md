# Intercept Example Docs Index

This directory documents the focused example schema for the current intercept
tree.

## Current Implemented Examples

- [INTERCEPT_PROBE.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/examples/INTERCEPT_PROBE.md)
  - path-only transport and RPC smoke test

- [INTERCEPT_DIRFD.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/examples/INTERCEPT_DIRFD.md)
  - remote dirfd semantics and descriptor classification

- [INTERCEPT_RW.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/examples/INTERCEPT_RW.md)
  - sequential remote file I/O and `lseek()`

- [INTERCEPT_OFFSET.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/examples/INTERCEPT_OFFSET.md)
  - offset-stable remote reads through `pread64()`

- [INTERCEPT_HTTP.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/examples/INTERCEPT_HTTP.md)
  - mixed local-socket and remote-filesystem integration story

## Schema Rule

Each example should validate one contract:

1. transport/path probes
2. dirfd policy
3. sequential file I/O
4. offset-stable positioned I/O
5. mixed local/remote integration

Do not keep growing a focused example into a broad regression bucket. Add a
new example when the next syscall or policy slice has a distinct contract.

## Planned Example Ideas

These are the next natural additions once the guest intercept layer grows:

- `intercept-fcntl`
  - remote fd control and validation behavior

- `intercept-writev`
  - vectored I/O on tracked remote file descriptors

- `intercept-session`
  - transport reset and remote-fd lifetime semantics

- `intercept-http-static`
  - realistic remote-file serving over local guest sockets
