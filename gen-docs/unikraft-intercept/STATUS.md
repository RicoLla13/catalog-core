# Intercept Status

This document is the single place for the current guest-side status.

## Current Coverage

Implemented guest-side intercept syscalls:

- `access()`
- `openat()`
- `close()`
- `newfstatat()`
- `fstat()`
- `lseek()`
- `pread64()`
- `read()`
- `write()`

Tracked remote fd rule:

- `read()`, `pread64()`, `write()`, `close()`, `fstat()`, and `lseek()` only
  intercept tracked remote file descriptors returned by intercepted `openat()`

Descriptor model status:

- tracked remote fds live in an intercept-owned metadata table
- guest-visible remote fd numbers are allocated independently from server-side
  remote fd numbers
- tracked remote fds are classified from post-open remote `fstat()` metadata,
  not only from `O_DIRECTORY`

## Active Examples

Active focused samples:

1. `intercept-probe`
   - path-only transport and RPC smoke test
2. `intercept-dirfd`
   - remote dirfd classification and `ENOTDIR` policy
3. `intercept-rw`
   - sequential remote file I/O and `lseek()`
4. `intercept-offset`
   - offset-stable `pread64()`
5. `intercept-http`
   - mixed local guest sockets plus intercept preflight

## Known Limits

Current implementation limits:

- one synchronous RPC in flight at a time
- fixed stack request/reply buffers
- single-fragment replies only
- no `writev()` intercept
- no `fcntl()` intercept
- no `dup()` integration for remote fds
- no `poll()` / `epoll()` integration for remote fds
- reconnect semantics for remote fds are not yet defined as a protocol
  contract

Current protocol caveat:

- the existing syscall-server protocol still defines `pread.offset` as XDR
  `long`
- guest `pread64()` is therefore currently limited to signed 32-bit offsets
  until the protocol is upgraded to a true 64-bit wire format

## Current Order Of Work

Implemented:

1. authoritative remote file-vs-directory tracking
2. guest-side `lseek()`
3. guest-side `pread64()`

Next:

1. guest-side `fcntl()`
2. guest/server session contract for remote fd lifetime
3. `writev()` protocol and guest/server implementation
4. scalable remote I/O beyond the current fixed RPC buffers
5. thread-safe RPC and fd-table serialization

For the server-side view of those items, especially fd-token/session semantics,
see
[SERVER_REQUIREMENTS.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/SERVER_REQUIREMENTS.md).
