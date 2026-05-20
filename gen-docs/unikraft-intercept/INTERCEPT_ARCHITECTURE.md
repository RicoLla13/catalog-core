# Intercept Architecture

This document explains the current intercept design at the policy and system
level.

## 1. Big Picture

The intercept library is a Unikraft-side client for a host syscall server.

Current flow:

1. guest code issues a syscall
2. a hooked Unikraft syscall path asks the intercept layer first
3. the intercept layer decides whether the call belongs to the remote model
4. if yes, it encodes an ONC RPC/XDR request
5. the transport sends the request to the host syscall server
6. the guest decodes the reply and returns a Linux-style result
7. otherwise the normal local Unikraft path runs

Current guest-side coverage:

- `access()`
- `openat()`
- `close()`
- `newfstatat()`
- `fstat()`
- `read()`
- `write()`

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
- `repos/unikraft/lib/intercept/rpc/rpc_write.c`
- `repos/unikraft/lib/intercept/rpc/rpc_internal.h`

Hook sites outside the library:

- `repos/unikraft/lib/posix-vfs/syscalls.c`
- `repos/unikraft/lib/posix-fdtab/fdtab.c`
- `repos/unikraft/lib/posix-fdio/fd-shim.c`

## 3. Public API

When enabled, `include/uk/intercept.h` exposes:

- `uk_intercept_boot_init(...)`
- `uk_intercept_access(...)`
- `uk_intercept_openat(...)`
- `uk_intercept_close(...)`
- `uk_intercept_newfstatat(...)`
- `uk_intercept_fstat(...)`
- `uk_intercept_read(...)`
- `uk_intercept_write(...)`

When disabled, the wrappers become inline stubs that return `-ENOTSUP`.

That allows hook sites to call intercept unconditionally and fall back to the
normal local implementation when intercept does not claim the syscall.

## 4. Boot Lifecycle

Boot init:

- zeroes the remote-fd bitmap
- initializes transport state
- optionally runs a boot-time RPC probe, depending on menuconfig policy
- sets `intercept_ready = 1`

Shutdown:

- resets the transport state

The library is registered with `uk_late_initcall(...)`, so it becomes active
only after late boot initialization.

### 4.1 Transport connection policy

Menuconfig exposes three policies:

- `Connect on first RPC`
- `Best-effort connect during boot`
- `Require successful connect during boot`

The eager modes now validate the host with an ONC RPC `NULLPROC` probe instead
of trusting a bare TCP `connect()`.

## 5. Remote FD Model

The current remote fd design is intentionally narrow.

Descriptors returned by intercepted `openat()` are:

- treated as remote-only
- tracked in a bitmap owned by the intercept layer
- not installed as rich objects in Unikraft's local file model

That bitmap currently answers one question:

- "is this fd remote?"

This is enough for the current staged subset:

- `close()`
- `fstat()`
- `read()`
- `write()`

All four only intercept tracked remote fds. Non-remote fds still follow the
normal local Unikraft path.

## 6. Current Syscall Semantics

### 6.1 `access()`

- path-based remote check
- falls back locally on `-ENOTSUP`

### 6.2 `openat()`

- absolute paths force remote `dirfd = AT_FDCWD`
- relative paths allow `AT_FDCWD` or a tracked remote directory fd
- success returns a new remote fd and marks it in the bitmap

### 6.3 `close()`

- only intercepted for tracked remote fds

### 6.4 `newfstatat()`

- path-based metadata lookup
- policy mirrors `openat()` for `dirfd`
- intended foundation for `stat()` / `lstat()` style behavior

### 6.5 `fstat()`

- metadata lookup on an already-open tracked remote fd

### 6.6 `read()` / `write()`

- only intercepted for tracked remote fds
- local socket or local file fds still go through the standard guest path

## 7. Current Example Workflow

The current examples split the old single-sample story into three pieces:

- `intercept-simple/main.c`
  - `access("/tmp", F_OK)`
  - `access("/tmp/intercept-simple-missing", F_OK)`

- `intercept-fs/main.c`
  - `access("/tmp", F_OK)`
  - `openat(AT_FDCWD, "/tmp", O_RDONLY | O_DIRECTORY, 0)`
  - `openat(dirfd, "intercept-fs.txt", O_CREAT | O_TRUNC | O_WRONLY, 0644)`
  - two `write()` calls
  - `close(fd)`
  - `fstatat(dirfd, "intercept-fs.txt", &st, 0)`
  - `openat(dirfd, "intercept-fs.txt", O_RDONLY, 0)`
  - `fstat(fd, &st)`
  - `read(fd, ...)`
  - `close(fd)`
  - `close(dirfd)`

- `intercept-http/server.c`
  - `access("/tmp", F_OK)` preflight through intercept
  - local guest HTTP socket flow through lwIP

## 8. Current Limits

- one synchronous RPC in flight at a time
- fixed stack request/reply buffers
- single-fragment replies only
- no `writev()` intercept
- no `dup()` integration for remote fds
- no `poll()` / `epoll()` integration for remote fds
- remote descriptors are not full first-class Unikraft file objects

## 9. Architectural Direction

The current design is still a staged bring-up model.

For larger applications, the next architectural step is not just "add more
syscalls". It is to move from:

- remote fd bitmap

to something closer to:

- remote descriptor metadata table
- per-fd backend type
- cleaner mixed local/remote cross-fd behavior

That matters especially for `intercept-http` once it starts serving remote
files. See `HTTP_SERVER_ROADMAP.md`.
