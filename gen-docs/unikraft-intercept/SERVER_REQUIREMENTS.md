# Server Requirements for Intercept

This document captures the requirements that the guest intercept side should be
able to demand from the syscall-server implementation.

It exists to make the guest/server contract explicit before more features are
added.

## 0. Server Worklist Summary

If this document is being used as a handoff for syscall-server work, the
highest-priority fd-related items are:

1. choose and document the remote-fd session model
   - recommended short-term choice: session-bound remote fds
2. make the fd token contract explicit
   - remote fds are server-issued session tokens, not guest fd numbers
3. guarantee strict fd validation on every fd-based RPC
   - invalid tokens must fail cleanly with `EBADF`
4. upgrade `pread64()` to a true 64-bit wire contract
   - change protocol `pread.offset` from XDR `long` to XDR `hyper`
5. prepare for upcoming guest-side `fcntl()`
   - define which commands are supported for remote regular files first
6. prepare for future `writev()`
   - decide how vectored payloads will be encoded and validated

Why this order:

- session semantics and fd token rules define the correctness envelope for all
  later fd-based syscalls
- validation rules prevent stale-token or bad-translation bugs from leaking
  into host syscalls
- `pread64()` is already implemented guest-side but still constrained by the
  old protocol field width
- `fcntl()` and `writev()` are the next practical compatibility steps after the
  current guest-side coverage

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

Operational interpretation:

- the guest may show fd `4` while the server internally maps that request to
  some unrelated host fd
- the guest-visible fd number must never be treated as the server-side object
  identity
- the server must be free to translate from remote token to host fd through its
  own session-scoped table

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
- `pread64()` validates translated `fd`
- `write()` validates translated `fd`
- `fstat()` validates translated `fd`
- `lseek()` validates translated `fd`
- `newfstatat()` validates translated `dirfd`

If translation fails, the server should return:

- `result = -1`
- `err = EBADF`

without issuing the host syscall.

Reason:

- the guest now performs more local policy checks, but fd-token correctness is
  still ultimately enforced at the server translation boundary
- every fd-based leaf must fail in the same way for bad remote tokens so the
  guest sees a stable contract

## 5. Reuse and Recycling

The server should document whether remote fd tokens may be reused.

If reuse is allowed, the contract must state:

- whether reuse can happen immediately after close
- whether reuse is scoped per connection or per session
- whether stale guest use of a previously closed fd can accidentally target a
  new server object

The safest model is:

- no accidental stale-fd aliasing within a live session

Practical recommendation:

- do not immediately recycle remote fd tokens within a live session unless the
  server can prove stale uses cannot alias a newly opened object
- if immediate reuse is kept for implementation simplicity, document it
  explicitly and treat it as part of the protocol contract

## 6. Future Feature Requirements

Before the guest can rely on broader fd behavior, the server side should be
prepared to support:

- `lseek()`
- `pread64()`
- `writev()`
- `fcntl()`
- `dup()` / `F_DUPFD`-style duplication semantics
- explicit session/reset behavior for all fd mappings

## 6.0 Upcoming Guest Expectation: `fcntl()` Scope

The next likely guest-side fd-control work is a narrow `fcntl()` subset for
tracked remote regular files.

Recommended first supported command set:

- `F_GETFL`
- `F_SETFL`
- `F_GETFD`
- `F_SETFD`

Why this subset first:

- it gives the guest basic fd-state queries and flag updates without forcing
  descriptor-sharing semantics immediately
- it avoids mixing in duplication behavior such as `F_DUPFD`, which would need
  shared descriptor state rather than per-fd snapshots

Server-side preparation request:

- decide which of the above commands are supported for remote regular files
- define uniform error behavior for unsupported commands
- document whether any command is intentionally rejected for remote
  descriptors even if the host kernel supports it

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
