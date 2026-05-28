# Server Requirements for Intercept

This document captures the requirements that the guest intercept side should be
able to demand from the syscall-server implementation.

It exists to make the guest/server contract explicit before more features are
added.

## 1. Connection and Session Semantics

The most important open contract is what happens when the guest transport
disconnects and reconnects.

The server implementation should define one of these models explicitly:

1. session-bound remote fds
2. reconnect-stable remote fds

### 1.1 Session-bound remote fds

If remote fds are valid only for the lifetime of one TCP/RPC session, the
server must guarantee:

- all remote fd mappings are discarded when the connection closes
- a new connection starts with a fresh remote-fd namespace
- the guest can detect that previously issued remote fds are now invalid

If this model is chosen, the guest side should invalidate all tracked remote
fds on transport reset.

### 1.2 Reconnect-stable remote fds

If remote fds are expected to survive reconnect, the server must guarantee:

- a stable session identity across reconnect
- stable remote fd mappings for that session
- unambiguous handling of duplicate reconnect attempts
- explicit rules for when a session expires and its fds become invalid

Without those guarantees, reconnect safety is undefined.

## 2. FD Namespace Contract

The server must treat remote fds as server-side session identifiers, not as
guest-visible fd numbers.

The guest may allocate its own visible fd numbers independently.

Server requirements:

- every successful `openat()` returns a server-managed remote fd token
- all fd-based RPCs accept only those server-issued remote fd tokens
- the mapping must remain internally consistent until close or session teardown
- invalid remote fds must fail cleanly with `EBADF`

## 3. File Type / Metadata Expectations

The guest will need authoritative file type information for stronger remote-fd
policy.

The server should support enough metadata to distinguish at least:

- regular file
- directory
- symlink, if `lstat`-style behavior is expected later

Today the guest can use `fstat()` / `newfstatat()`, but richer descriptor-aware
dispatch will eventually need the server contract to make file type reliable.

## 4. Validation Requirements

The server should validate all translated fds before calling host syscalls.

Minimum expectation:

- `openat()` validates translated `dirfd`
- `close()` validates translated `fd`
- `read()` validates translated `fd`
- `write()` validates translated `fd`
- `fstat()` validates translated `fd`
- `newfstatat()` validates translated `dirfd`

If translation fails, the server should return:

- `result = -1`
- `err = EBADF`

without issuing the host syscall.

## 5. Reuse and Recycling

The server should document whether remote fd tokens may be reused.

If reuse is allowed, the contract must state:

- whether reuse can happen immediately after close
- whether reuse is scoped per connection or per session
- whether stale guest use of a previously closed fd can accidentally target a
  new server object

The safest model is:

- no accidental stale-fd aliasing within a live session

## 6. Future Feature Requirements

Before the guest can rely on broader fd behavior, the server side should be
prepared to support:

- `lseek()`
- `pread64()`
- `writev()`
- `fcntl()`
- `dup()` / `F_DUPFD`-style duplication semantics
- explicit session/reset behavior for all fd mappings

## 6.1 Immediate Protocol Upgrade Request: True 64-bit `pread64()` Offsets

The current protocol still defines `pread.offset` as XDR `long`.

Current impact:

- the guest-side `pread64()` hook is implemented
- but the guest must currently stay within signed 32-bit offsets when talking
  to the existing server
- this is a protocol compatibility limitation, not the desired long-term
  contract

Why this should be fixed:

- `pread64()` is expected to support large-file offsets beyond 2 GiB
- the guest API already exposes an `off_t`-based positioned-read interface
- the guest intercept layer already treats `lseek()` offsets as 64-bit on the
  wire
- keeping `pread64()` at 32-bit while `lseek()` is 64-bit creates an avoidable
  semantic mismatch
- realistic follow-on workloads such as large assets, disk images, or large log
  files will eventually need true 64-bit positioned reads

Requested server/protocol change:

1. update `repos/syscall-server/src/protocol/protocol.x`
   - change `pread_request.offset` from XDR `long` to XDR `hyper`
2. regenerate the protocol bindings used by the syscall server
3. update the server-side `pread` implementation to consume the 64-bit offset
   contract explicitly
4. keep result/error behavior unchanged

Expected compatibility outcome:

- after the protocol upgrade, guest `pread64()` should accept the same offset
  range that the guest `off_t` can represent on the target
- positioned reads should remain offset-stable with respect to the tracked fd's
  current file position
- the guest-side temporary 32-bit offset guard can then be removed

Acceptance criteria for the server-side handoff:

- `pread64(fd, buf, count, large_offset)` works correctly for offsets beyond
  signed 32-bit range
- no field-order mismatch remains between guest and server logs
- small-offset behavior remains unchanged for current samples
- the `intercept-offset` sample continues to pass, and a future large-offset
  variant can be added on top

## 7. Immediate Takeaway

The guest intercept side should not assume reconnect-safe remote fd state until
the server explicitly guarantees it.

That requirement should be treated as part of the protocol contract, not as an
incidental property of the current implementation.
