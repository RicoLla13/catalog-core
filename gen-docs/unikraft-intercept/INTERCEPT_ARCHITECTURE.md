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
- `lseek()`
- `pread64()`
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
- `repos/unikraft/lib/intercept/rpc/rpc_pread.c`
- `repos/unikraft/lib/intercept/rpc/rpc_read.c`
- `repos/unikraft/lib/intercept/rpc/rpc_write.c`
- `repos/unikraft/lib/intercept/rpc/rpc_lseek.c`
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
- `uk_intercept_lseek(...)`
- `uk_intercept_pread(...)`
- `uk_intercept_read(...)`
- `uk_intercept_write(...)`

When disabled, the wrappers become inline stubs that return `-ENOTSUP`.

That allows hook sites to call intercept unconditionally and fall back to the
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
- tracked in an intercept-owned metadata table keyed by guest-visible fd
- not installed as rich objects in Unikraft's local file model

Each table entry currently stores:

- descriptor backend kind:
  - remote file
  - remote directory
- remote server fd
- open flags and mode
- cached offset field reserved for follow-on work

Current caveat:

- tracked remote fds returned by `openat()` are now classified from a post-open
  remote `fstat()`
- that makes the file-vs-directory tag authoritative for the current tracked-fd
  model
- this costs one extra RPC after each successful remote `openat()`
- richer descriptor metadata beyond file-vs-directory is still deferred

This is enough for the current staged subset:

- `close()`
- `fstat()`
- `lseek()`
- `pread64()`
- `read()`
- `write()`

All six only intercept tracked remote fds. Non-remote fds still follow the
normal local Unikraft path.

## 6. Current Syscall Semantics

### 6.1 `access()`

- path-based remote check
- falls back locally on `-ENOTSUP`

### 6.2 `openat()`

- absolute paths force remote `dirfd = AT_FDCWD`
- relative paths allow `AT_FDCWD` or a tracked remote directory fd
- success returns a new remote fd and registers it in the intercept fd table

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

### 6.7 `lseek()`

- only intercepted for tracked remote fds
- updates the intercept table's cached offset field on success
- local socket or local file fds still go through the standard guest path

### 6.8 `pread64()`

- only intercepted for tracked remote fds
- uses an explicit offset and does not mutate the tracked fd's cached offset
- local socket or local file fds still go through the standard guest path

## 7. Current Example Workflow

The active examples are now split by contract:

- `intercept-probe/main.c`
  - `access("/tmp", F_OK)`
  - `access("/tmp/intercept-probe-missing", F_OK)`

- `intercept-dirfd/main.c`
  - `access("/tmp", F_OK)`
  - `openat(AT_FDCWD, "/tmp", O_RDONLY | O_DIRECTORY, 0)`
  - `openat(AT_FDCWD, "/tmp", O_RDONLY, 0)`
  - `openat(classified_dirfd, "intercept-dirfd.txt", O_CREAT | O_TRUNC | O_WRONLY, 0644)`
  - `close(fd)`
  - `fstatat(strict_dirfd, "intercept-dirfd.txt", &st, 0)`
  - `fstatat(classified_dirfd, "intercept-dirfd.txt", &st, 0)`
  - `openat(strict_dirfd, "intercept-dirfd.txt", O_RDONLY, 0)`
  - `fstatat(filefd, "intercept-dirfd-missing-child", &st, 0)` -> `ENOTDIR`
  - `close(fd)`
  - `close(classified_dirfd)`
  - `close(strict_dirfd)`

- `intercept-rw/main.c`
  - `access("/tmp", F_OK)`
  - `openat(AT_FDCWD, "/tmp/intercept-rw.txt", O_CREAT | O_TRUNC | O_RDWR, 0644)`
  - two `write()` calls
  - `lseek(fd, 0, SEEK_CUR)`
  - `lseek(fd, 8, SEEK_SET)`
  - `read(fd, ...)`
  - `lseek(fd, 0, SEEK_SET)`
  - `fstat(fd, &st)`
  - `read(fd, ...)`
  - `close(fd)`

- `intercept-offset/main.c`
  - `access("/tmp", F_OK)`
  - `openat(AT_FDCWD, "/tmp/intercept-offset.txt", O_CREAT | O_TRUNC | O_RDWR, 0644)`
  - `write(fd, ...)`
  - `lseek(fd, 0, SEEK_CUR)`
  - `pread(fd, ..., 0)`
  - `pread(fd, ..., 7)`
  - `lseek(fd, 0, SEEK_CUR)` unchanged
  - `close(fd)`

- `intercept-http/server.c`
  - `access("/tmp", F_OK)` preflight through intercept
  - local guest HTTP socket flow through lwIP

## 8. Current Limits

- one synchronous RPC in flight at a time
- fixed stack request/reply buffers
- single-fragment replies only
- reconnect semantics for remote fds are not yet defined as a protocol
  contract
- no `writev()` intercept
- no `dup()` integration for remote fds
- no `poll()` / `epoll()` integration for remote fds
- remote descriptors are not full first-class Unikraft file objects

Deferred implementation requirements:

- thread-safe RPC serialization and fd-table access
  - current assumption: single-threaded guest workloads
- chunked or otherwise scalable remote I/O beyond the current fixed RPC buffers
- explicit guest/server session contract for remote fd lifetime across
  transport reset or reconnect
- shared descriptor state for future `dup()` / offset semantics

## 9. Architectural Direction

The current design is still a staged bring-up model.

For larger applications, the next architectural step is not just "add more
syscalls". It is to move from:

- minimal remote fd metadata table

to something closer to:

- remote descriptor metadata table
- per-fd backend type
- cleaner mixed local/remote cross-fd behavior

That matters especially for `intercept-http` once it starts serving remote
files. See `HTTP_SERVER_ROADMAP.md`.

### 9.1 Authoritative File-vs-Directory Tracking

Correctly distinguishing remote regular files from remote directories was the
next descriptor-model correctness task.

Why it matters:

- relative `openat()` and `newfstatat()` should eventually reject invalid
  remote dirfds locally
- the current table only has a heuristic backend tag derived from open flags
- that heuristic is not authoritative enough to enforce `ENOTDIR` safely

Possible fixes considered:

1. post-open `fstat()` classification
   - after a successful remote `openat()`, issue remote `fstat()`
   - classify the guest fd from `st_mode`
   - simplest incremental path, but adds an extra RPC on open

2. extend `openat()` RPC response with file type metadata
   - have the server return file type bits together with the remote fd
   - avoids a second RPC
   - requires a protocol change on both guest and server

3. add a dedicated descriptor metadata RPC
   - keep `openat()` lean
   - fetch richer per-fd metadata only when needed
   - useful if the table later needs more than just file type

Chosen direction:

- current implementation: post-open `fstat()` classification
- longer term: extend the RPC contract so `openat()` can return authoritative
  descriptor metadata directly

### 9.2 Current Implementation Plan

Status markers:

- `[done]` implemented in the current guest intercept layer
- `[pending]` agreed next step, not started

1. `[done]` authoritative remote file-vs-directory tracking
   - classify each tracked remote fd from remote `fstat().st_mode` after
     successful `openat()`
   - tighten local remote-dirfd validation once the guest table stores real
     type information
2. `[done]` guest-side `lseek()`
3. `[done]` guest-side `pread64()`
4. `[pending]` guest-side `fcntl()`
5. `[pending]` guest/server session contract for remote fd lifetime
   - preferred short-term policy: session-bound remote fds with guest-side
     invalidation on transport reset
6. `[pending]` `writev()` protocol and guest/server implementation
