# Unikraft Intercept Docs Index

This directory contains the generated documentation for the current Unikraft
intercept implementation.

Start here:

- [INTERCEPT_ARCHITECTURE.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/INTERCEPT_ARCHITECTURE.md)
  - overall design
  - boot lifecycle
  - remote fd model
  - supported syscalls
  - current limits

- [INTERCEPT_RPC.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/INTERCEPT_RPC.md)
  - ONC RPC/XDR constants
  - transport behavior
  - shared RPC core
  - per-syscall RPC leaf behavior

- [INTERCEPT_HOOKS.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/INTERCEPT_HOOKS.md)
  - where the intercept layer hooks into Unikraft
  - how the current samples exercise those hooks
  - extension guidance

- [HTTP_SERVER_ROADMAP.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/HTTP_SERVER_ROADMAP.md)
  - what changes are needed to support `intercept-http` beyond its current
    fixed-response server
  - syscall roadmap
  - architectural changes beyond just adding more RPC leaves

- [SERVER_REQUIREMENTS.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/SERVER_REQUIREMENTS.md)
  - requirements the guest side should be able to demand from the syscall
    server
  - connection/session semantics for remote fds
  - validation and mapping guarantees

Current guest intercept coverage:

- `access()`
- `openat()`
- `close()`
- `newfstatat()`
- `fstat()`
- `lseek()`
- `pread64()`
- `read()`
- `write()`

Short status summary:

- remote file descriptors are now tracked in an intercept-owned metadata table
  keyed by guest-visible fd
- `read()`, `pread64()`, `write()`, `close()`, `fstat()`, and `lseek()` only
  apply to tracked remote fds
- `newfstatat()` is the current path-based metadata primitive
- eager boot-connect modes use an ONC RPC probe, not just raw TCP connect
