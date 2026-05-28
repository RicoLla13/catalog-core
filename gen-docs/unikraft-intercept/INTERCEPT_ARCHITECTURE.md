# Intercept Architecture

This document explains the current intercept design at the policy and system
level. For current status, active examples, and next steps, read
[STATUS.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/STATUS.md)
first.

## 1. Big Picture

The intercept library is a Unikraft-side client for a host syscall server.

High-level flow:

1. guest code issues a syscall
2. a hooked Unikraft syscall path asks the intercept layer first
3. the intercept layer decides whether the call belongs to the remote model
4. if yes, it encodes an ONC RPC/XDR request
5. the transport sends the request to the host syscall server
6. the guest decodes the reply and returns a Linux-style result
7. otherwise the normal local Unikraft path runs

## 2. File Layout

Public API:

- `repos/unikraft/lib/intercept/include/uk/intercept.h`

Policy and transport:

- `repos/unikraft/lib/intercept/intercept.c`
- `repos/unikraft/lib/intercept/transport.c`
- `repos/unikraft/lib/intercept/intercept_internal.h`

RPC layer:

- `repos/unikraft/lib/intercept/rpc/rpc_core.c`
- `repos/unikraft/lib/intercept/rpc/rpc_xdr.c`
- `repos/unikraft/lib/intercept/rpc/rpc_probe.c`
- `repos/unikraft/lib/intercept/rpc/rpc_access.c`
- `repos/unikraft/lib/intercept/rpc/rpc_openat.c`
- `repos/unikraft/lib/intercept/rpc/rpc_close.c`
- `repos/unikraft/lib/intercept/rpc/rpc_newfstatat.c`
- `repos/unikraft/lib/intercept/rpc/rpc_fstat.c`
- `repos/unikraft/lib/intercept/rpc/rpc_read.c`
- `repos/unikraft/lib/intercept/rpc/rpc_pread.c`
- `repos/unikraft/lib/intercept/rpc/rpc_write.c`
- `repos/unikraft/lib/intercept/rpc/rpc_lseek.c`
- `repos/unikraft/lib/intercept/rpc/rpc_internal.h`

Hook sites outside the library:

- `repos/unikraft/lib/posix-vfs/syscalls.c`
- `repos/unikraft/lib/posix-fdtab/fdtab.c`
- `repos/unikraft/lib/posix-fdio/fd-shim.c`

## 3. Public API Shape

When enabled, `include/uk/intercept.h` exposes wrappers such as:

- `uk_intercept_access(...)`
- `uk_intercept_openat(...)`
- `uk_intercept_close(...)`
- `uk_intercept_newfstatat(...)`
- `uk_intercept_fstat(...)`
- `uk_intercept_lseek(...)`
- `uk_intercept_pread(...)`
- `uk_intercept_read(...)`
- `uk_intercept_write(...)`

When disabled, the wrappers become inline stubs returning `-ENOTSUP`.

That lets hook sites call intercept unconditionally and fall back to the
normal local implementation when intercept does not claim the syscall.

## 4. Boot Lifecycle

Boot init:

- zeroes the remote-fd metadata table
- initializes transport state
- optionally runs a boot-time RPC probe, depending on menuconfig policy
- sets `intercept_ready = 1`

Shutdown:

- resets the transport state

The library is registered with `uk_late_initcall(...)`, so it becomes active
only after late boot initialization.

### 4.1 Transport Connection Policy

Menuconfig exposes three policies:

- `Connect on first RPC`
- `Best-effort connect during boot`
- `Require successful connect during boot`

The eager modes validate the host with an ONC RPC `NULLPROC` probe instead of
trusting a bare TCP `connect()`.

## 5. Remote FD Model

The current remote fd design is intentionally narrow.

Descriptors returned by intercepted `openat()` are:

- treated as remote-only
- tracked in an intercept-owned metadata table keyed by guest-visible fd
- not installed as rich objects in Unikraft's local file model

Each table entry currently stores:

- descriptor backend kind:
  - remote file
  - remote directory
- remote server fd
- open flags and mode
- cached offset field reserved for follow-on work

### 5.1 Authoritative File-vs-Directory Tracking

Tracked remote fds are now classified from a post-open remote `fstat()`.

Why that matters:

- relative `openat()` and `newfstatat()` need reliable local dirfd policy
- `O_DIRECTORY` alone is only a heuristic about caller intent
- server-derived `st_mode` makes the current file-vs-directory tag
  authoritative for the tracked-fd model

Tradeoff:

- each successful remote `openat()` currently costs one extra RPC for the
  classification `fstat()`

Longer-term direction:

- move file-type or richer descriptor metadata directly into the `openat()`
  response or a dedicated metadata RPC

## 6. Current Syscall Semantics

### 6.1 `access()`

- path-based remote check
- falls back locally on `-ENOTSUP`

### 6.2 `openat()`

- absolute paths force remote `dirfd = AT_FDCWD`
- relative paths allow `AT_FDCWD` or a tracked remote directory fd
- success returns a new guest-visible fd backed by a server-side remote fd

### 6.3 `close()`

- only intercepted for tracked remote fds

### 6.4 `newfstatat()`

- path-based metadata lookup
- policy mirrors `openat()` for `dirfd`

### 6.5 `fstat()`

- metadata lookup on a tracked remote fd

### 6.6 `read()` / `write()`

- only intercepted for tracked remote fds
- local socket or local file fds still go through the standard guest path

### 6.7 `lseek()`

- only intercepted for tracked remote fds
- updates the intercept table's cached offset field on success

### 6.8 `pread64()`

- only intercepted for tracked remote fds
- uses an explicit offset and does not mutate the tracked fd's cached offset
- currently limited by the server wire contract to signed 32-bit offsets
  despite the guest-side API shape

## 7. Architectural Limits

Current structural limits:

- one synchronous RPC in flight at a time
- fixed request/reply buffers
- single-fragment replies only
- remote descriptors are not full first-class Unikraft file objects
- reconnect semantics are not yet a formal protocol contract

Deferred architectural work:

- thread-safe RPC serialization and fd-table access
- scalable remote I/O beyond the current fixed RPC buffers
- explicit session semantics for remote fd lifetime
- shared descriptor state for future `dup()` and offset-sharing behavior
- richer remote descriptor metadata beyond file-vs-directory
