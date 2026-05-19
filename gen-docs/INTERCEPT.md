# Intercept Library: Current Design and Code Walkthrough

This document explains the current guest-side intercept library in
`repos/unikraft/lib/intercept/`, how the code is structured, how syscalls move
through it, how the RPC layer works, how the transport works, how file
descriptors are handled, and what was changed in the syscall shims to make the
current model work.

The goal is not to describe an abstract design. This document describes the
code that currently exists in this tree.

## 1. Big Picture

The intercept library is a Unikraft-side client for a host syscall server.

At runtime, the path is:

1. a guest syscall is invoked
2. the relevant Unikraft syscall implementation calls into `lib/intercept`
3. `lib/intercept` decides whether the call belongs to the remote model
4. if yes, it encodes an ONC RPC/XDR request
5. it sends that request over a TCP transport to the host server
6. it receives and decodes the reply
7. it returns a Linux-style result back into the guest syscall path

At the moment, the guest-side intercept library supports:

- `access()`
- `openat()`
- `close()`
- `read()`
- `write()`

The current file-I/O model is deliberately restricted:

- the guest only uses remote file descriptors for the syscalls implemented so
  far
- descriptors returned by intercepted `openat()` are treated as remote-only
- `read()`, `write()`, and `close()` only intercept those tracked remote fds
- local Unikraft behavior is still available when intercept is disabled, or
  when a syscall wrapper returns `-ENOTSUP`

This is a staged bring-up model, not a complete replacement of Unikraft's fd
namespace.

## 2. File Layout

The active files are:

### Public API

- `repos/unikraft/lib/intercept/include/uk/intercept.h`

### Internal library code

- `repos/unikraft/lib/intercept/intercept.c`
- `repos/unikraft/lib/intercept/transport.c`
- `repos/unikraft/lib/intercept/intercept_internal.h`
- `repos/unikraft/lib/intercept/Makefile.uk`

### RPC subsystem

- `repos/unikraft/lib/intercept/rpc/rpc_internal.h`
- `repos/unikraft/lib/intercept/rpc/rpc_core.c`
- `repos/unikraft/lib/intercept/rpc/rpc_xdr.c`
- `repos/unikraft/lib/intercept/rpc/rpc_access.c`
- `repos/unikraft/lib/intercept/rpc/rpc_openat.c`
- `repos/unikraft/lib/intercept/rpc/rpc_close.c`
- `repos/unikraft/lib/intercept/rpc/rpc_read.c`
- `repos/unikraft/lib/intercept/rpc/rpc_write.c`

### Guest syscall hook points outside the library

- `repos/unikraft/lib/posix-vfs/syscalls.c`
- `repos/unikraft/lib/posix-fdtab/fdtab.c`
- `repos/unikraft/lib/posix-fdio/fd-shim.c`

## 3. Build Wiring

`repos/unikraft/lib/intercept/Makefile.uk` is the build entry point.

It pulls in:

- `intercept.c`
- `transport.c`
- `rpc/rpc_core.c`
- `rpc/rpc_xdr.c`
- one RPC leaf file per syscall currently supported

That split matters. The old monolithic `rpc.c` is gone. The current structure
separates:

- policy and fd ownership in `intercept.c`
- transport in `transport.c`
- generic RPC framing in `rpc_core.c`
- XDR helpers in `rpc_xdr.c`
- syscall-specific request/response logic in `rpc_<syscall>.c`

This is the key reason the RPC layer is now maintainable as you add more
syscalls.

## 4. Public API: `include/uk/intercept.h`

This header is the public face of the intercept library.

When `CONFIG_LIBINTERCEPT=y`, it exposes:

- `uk_intercept_boot_init(...)`
- `uk_intercept_access(...)`
- `uk_intercept_openat(...)`
- `uk_intercept_close(...)`
- `uk_intercept_read(...)`
- `uk_intercept_write(...)`

When `CONFIG_LIBINTERCEPT=n`, all syscall wrappers become inline stubs that
return `-ENOTSUP`.

That design is important:

- hook sites in the rest of Unikraft can call the intercept layer
  unconditionally
- if the intercept feature is disabled, the wrappers return `-ENOTSUP`
- the caller then falls back to the normal Unikraft path

So "disabled in menuconfig" means "normal Unikraft behavior".

## 5. Internal Interface: `intercept_internal.h`

This header is intentionally small.

It exposes:

- transport helpers:
  - `uk_intercept_transport_init()`
  - `uk_intercept_transport_term()`
  - `uk_intercept_transport_send()`
  - `uk_intercept_transport_recv_exact()`
- per-syscall RPC entry points:
  - `uk_intercept_rpc_access()`
  - `uk_intercept_rpc_openat()`
  - `uk_intercept_rpc_close()`
  - `uk_intercept_rpc_read()`
  - `uk_intercept_rpc_write()`

This keeps the policy layer (`intercept.c`) separated from the wire-format
layer (`rpc/*`).

## 6. Policy Layer: `intercept.c`

`intercept.c` is the most important file conceptually. It decides:

- whether the intercept layer is ready
- whether a given fd belongs to the remote model
- whether a syscall should be remote or not
- how caller-visible `errno` should behave

### 6.1 Boot lifecycle

At boot:

- `uk_intercept_boot_init(...)`
  - zeroes the remote-fd bitmap
  - initializes the transport
  - sets `intercept_ready = 1`

At shutdown:

- `uk_intercept_boot_term(...)`
  - tears down the transport

The library is registered with:

- `uk_late_initcall(uk_intercept_boot_init, uk_intercept_boot_term);`

So the intercept layer becomes active only after late boot initialization.

### 6.2 `intercept_ready`

`intercept_ready` is a simple gate:

- if it is not set yet, wrappers return `-ENOTSUP`
- callers then use the normal local syscall path

This is the same fallback pattern used when the feature is disabled.

### 6.3 Remote fd tracking

The file descriptor model is based on:

- `UK_INTERCEPT_REMOTE_FD_MAX`
- `intercept_remote_fds[]`

`intercept_remote_fds[]` is a bitmap indexed by guest fd number.

The helper functions are:

- `uk_intercept_fd_in_range(fd)`
- `uk_intercept_is_remote_fd(fd)`

Current meaning:

- a bit set in the bitmap means "this fd is owned by the remote intercept
  model"
- only those fds are eligible for intercepted `read()`, `write()`, and
  `close()`

This bitmap is not a full fd table. It does not store metadata beyond
ownership. It only answers one question:

- "is this fd remote?"

That is enough for the current staged model.

### 6.4 Why a bitmap is still used in a remote-only model

Even though the current examples use remote-only file descriptors, the guest
still needs a way to decide whether:

- `close(fd)` should go to the server, or
- `read(fd)` and `write(fd)` should go to the server

The bitmap gives that answer.

The code does not attempt to merge remote fds into Unikraft's local fd table.
Instead, it tracks remote ownership separately.

### 6.5 `uk_intercept_access()`

This is the simplest wrapper.

Behavior:

- return `-ENOTSUP` if intercept is not ready
- return `-EFAULT` if `path == NULL`
- call `uk_intercept_rpc_access(path, mode)`
- preserve the caller-visible `errno` if the RPC call succeeds

Important point:

- the RPC layer returns Linux-style values, usually `>= 0` on success and
  `-errno` on failure
- on success, `errno` should not be clobbered by internal transport/RPC work

### 6.6 `uk_intercept_openat()`

This is the first wrapper that interacts with the remote-fd model.

Behavior:

1. if intercept is not ready: return `-ENOTSUP`
2. if `path == NULL`: return `-EFAULT`
3. decide which dirfd to send remotely:
   - if `path` is absolute, ignore caller `dfd` and force `AT_FDCWD`
   - if `path` is relative:
     - allow `AT_FDCWD`
     - otherwise require `dfd` to already be a tracked remote fd
4. call `uk_intercept_rpc_openat(...)`
5. if the returned fd is out of range, close it remotely and fail with
   `-EMFILE`
6. mark that returned fd as remote in the bitmap
7. preserve caller-visible `errno` on success

That is the core of the remote-only fd model.

Important semantic detail:

- relative `openat()` on a non-remote dirfd is rejected with `-EBADF`
- this prevents accidental mixing of local directory fds with remote path
  resolution

### 6.7 `uk_intercept_close()`

Behavior:

1. if intercept is not ready: return `-ENOTSUP`
2. if fd is not tracked as remote: return `-ENOTSUP`
3. call `uk_intercept_rpc_close(fd)`
4. on success, clear the fd bit from the remote bitmap
5. preserve caller-visible `errno` on success

Notice the layering:

- policy layer decides ownership
- RPC layer only performs the remote close

### 6.8 `uk_intercept_read()`

Behavior:

1. if intercept is not ready: return `-ENOTSUP`
2. if fd is not remote: return `-ENOTSUP`
3. if `buf == NULL` and `count != 0`: return `-EFAULT`
4. call `uk_intercept_rpc_read(fd, buf, count)`
5. preserve caller-visible `errno` on success

### 6.9 `uk_intercept_write()`

This mirrors `read()`.

Behavior:

1. if intercept is not ready: return `-ENOTSUP`
2. if fd is not remote: return `-ENOTSUP`
3. if `buf == NULL` and `count != 0`: return `-EFAULT`
4. call `uk_intercept_rpc_write(fd, buf, count)`
5. preserve caller-visible `errno` on success

## 7. Transport Layer: `transport.c`

`transport.c` is intentionally dumb.

It does not know:

- syscall numbers
- request structs
- XDR
- reply semantics
- remote fd ownership

It only knows how to maintain a TCP connection and move bytes.

### 7.1 Current transport model

The transport uses:

- one global socket fd
- one boolean `intercept_transport_connected`

This means:

- the current implementation assumes one synchronous request in flight at a
  time
- the same TCP connection is reused until failure or shutdown

This matches the current RPC design.

### 7.2 `uk_intercept_transport_init()`

Sets:

- `intercept_transport_fd = -1`
- `intercept_transport_connected = false`

It does not connect immediately. Connection is lazy.

### 7.3 `uk_intercept_transport_connect()`

This performs lazy connect:

- if already connected, return success
- create a socket
- parse `CONFIG_LIBINTERCEPT_REMOTE_IPV4`
- connect to `CONFIG_LIBINTERCEPT_REMOTE_PORT`
- store the connected socket globally

The remote endpoint is currently configured via menuconfig options.

### 7.4 `uk_intercept_transport_send_all()`

This is a blocking send loop.

It repeatedly calls `send()` until all bytes are transmitted or an error
occurs.

Behavior:

- `send() < 0` returns `-errno`
- `send() == 0` is treated as connection reset

### 7.5 `uk_intercept_transport_send()`

This is the public send entry point.

Behavior:

1. reject `NULL` or zero-length buffers with `-EINVAL`
2. ensure the connection exists
3. send all bytes
4. on failure, reset transport state

### 7.6 `uk_intercept_transport_recv_exact()`

This is the public receive entry point.

Behavior:

1. reject `NULL` or zero-length buffers with `-EINVAL`
2. ensure the connection exists
3. repeatedly call `recv()` until exactly `len` bytes are received
4. on error or EOF, reset transport state

Important point:

- transport does not understand message boundaries
- the RPC layer tells transport exactly how many bytes to read

### 7.7 No transport when disabled

If `CONFIG_LIBINTERCEPT_TRANSPORT_SOCKET` is off, the transport entry points
exist as stubs and return `-ENOTSUP`.

That keeps the layering intact and lets the upper layers fall back.

## 8. RPC Split: Why It Exists

The old monolithic `rpc.c` mixed:

- XDR primitives
- RPC framing
- request/response plumbing
- syscall-specific encoding
- syscall-specific decoding

That scales badly.

The current split is:

- `rpc_internal.h`: shared RPC types/constants
- `rpc_xdr.c`: field-level encode/decode helpers
- `rpc_core.c`: generic request/reply framing
- `rpc_<syscall>.c`: syscall-specific leaves

This means the shared core should stay mostly stable while new syscalls are
added as separate leaf files.

## 9. RPC Shared Header: `rpc_internal.h`

This header defines the shared RPC contract inside `rpc/`.

It contains:

- buffer-size constants
- ONC RPC constants
- syscall procedure numbers
- encode/decode cursor structs
- callback typedefs
- prototypes for:
  - primitive field helpers
  - string/opaque helpers
  - `rpc_call(...)`

### 9.1 Cursors

The two central structures are:

- `struct rpc_encode_cursor`
- `struct rpc_decode_cursor`

Each cursor contains:

- current position pointer
- end pointer

This prevents open-coded pointer arithmetic from spreading everywhere.

## 10. XDR Helpers: `rpc_xdr.c`

This file implements low-level encoding/decoding building blocks.

### 10.1 `rpc_put_u32()` / `rpc_get_u32()`

These are private helpers for:

- bounds checking
- endian conversion
- moving the cursor forward

They work in network byte order using `htonl()` / `ntohl()`.

### 10.2 `rpc_encode_u32()` / `rpc_decode_u32()`

These are the cursor-facing wrappers used throughout the RPC code.

### 10.3 `rpc_put_opaque()`

This encodes an XDR opaque field:

1. encode the payload length as a 32-bit integer
2. copy the payload bytes
3. append zero padding to 4-byte alignment

This helper is used for:

- strings encoded as XDR opaque data
- write payloads

### 10.4 `rpc_skip_opaque()`

This parses the length and skips over an opaque field without copying it.

Used in:

- reply verifier parsing in the common RPC core

### 10.5 `rpc_decode_opaque()`

This is the shared helper added once `read()` required variable-length reply
payloads.

Behavior:

1. read the XDR length
2. validate that enough bytes remain for the payload and padding
3. return a pointer to the payload bytes
4. advance the decode cursor over payload plus padding

Important design choice:

- it does not allocate
- it returns a pointer into the received reply buffer
- syscall-specific decode logic decides whether to copy that data elsewhere

### 10.6 `rpc_path_len()`

This is a bounded string-length helper using `strnlen()`.

It caps path scanning at:

- `UK_INTERCEPT_MAX_PATH_LEN + 1`

That lets the caller detect oversized paths cleanly.

## 11. Shared RPC Core: `rpc_core.c`

This file contains the generic ONC RPC request/reply machinery.

### 11.1 `rpc_xid`

`rpc_xid` is a global monotonically increasing transaction ID.

Current assumptions:

- one synchronous request at a time
- no concurrent in-flight multiplexing

This matches both the transport model and the current staged use case.

### 11.2 `rpc_put_call_header()`

This emits the standard ONC RPC call header:

- XID
- message type `CALL`
- RPC version
- program number
- program version
- procedure number
- AUTH_NONE credentials
- AUTH_NONE verifier

This is fully generic across all syscalls.

### 11.3 `rpc_read_accepted_reply()`

This parses the generic reply envelope.

Behavior:

1. read the TCP record marker
2. require the reply to be a single fragment
3. read the fragment payload
4. decode:
   - reply XID
   - message type
   - reply status
   - verifier flavor
   - verifier body
   - accept status
5. reject malformed replies with `-EPROTO`
6. return a decode cursor positioned at the procedure-specific payload

Important points:

- multi-fragment replies are not supported yet
- accepted replies must end in `RPC_SUCCESS`
- verifier must be `AUTH_NONE`

### 11.4 `rpc_call()`

This is the most important shared function in the RPC layer.

Inputs:

- syscall procedure number
- encode callback
- request argument pointer
- decode callback
- response object pointer

Behavior:

1. allocate fixed request and reply buffers on the stack
2. initialize the encode cursor
3. emit the generic call header
4. let the syscall-specific encoder append its payload
5. prepend the ONC RPC TCP record marker
6. send the request over transport
7. parse the accepted reply envelope
8. let the syscall-specific decoder parse its payload
9. reject trailing undecoded bytes with `-EPROTO`

This function is the shared heart of the new RPC model.

Every syscall leaf now uses it.

## 12. Syscall Leaf Files

Each syscall leaf contains:

- local request struct
- local response struct
- request encoder
- response decoder
- exported `uk_intercept_rpc_<syscall>()`

This keeps syscall logic isolated.

### 12.1 `rpc_access.c`

Implements:

- `path + mode` request
- `result + err` response

The wrapper returns:

- `resp.result` on success
- `-resp.err` on remote syscall failure
- `-EIO` if the server reported failure but no errno

### 12.2 `rpc_openat.c`

Implements:

- request:
  - `dirfd`
  - `path`
  - `flags`
  - `mode`
- response:
  - `fd`
  - `result`
  - `err`

The wrapper currently returns `resp.result`, which matches the server's client
fd mapping result.

Important note:

- the server returns the fd number to use as the remote handle
- the guest stores that fd in the remote-fd bitmap

### 12.3 `rpc_close.c`

Implements:

- request: `fd`
- response: `result + err`

No extra payload.

### 12.4 `rpc_read.c`

This is the first variable-length reply syscall.

Request:

- `fd`
- `count`

Response:

- opaque data payload
- `result`
- `err`

The decoder:

1. decodes the opaque data field
2. decodes `result`
3. decodes `err`
4. if `result > 0`, validates:
   - payload length is large enough
   - caller buffer is large enough
5. copies the returned bytes into the caller buffer

Important point:

- data copy happens in the syscall-specific leaf, not in the shared core
- this is exactly why keeping syscalls in leaf files is useful

### 12.5 `rpc_write.c`

This is the mirror of `read()`, but the variable-length payload lives in the
request instead of the reply.

Request:

- `fd`
- opaque data payload

Response:

- `result`
- `err`

The encoder uses `rpc_put_opaque()` to serialize the write buffer.

## 13. Where the Intercept Hooks Were Added Outside the Library

The intercept library alone is not enough. Unikraft's syscall paths had to be
adjusted so those wrappers are actually consulted.

### 13.1 `openat()` hook in `posix-vfs/syscalls.c`

The `openat()` syscall implementation now does:

1. call `uk_intercept_openat(dfd, path, flags, mode)`
2. if the return value is anything except `-ENOTSUP`, return it directly
3. otherwise continue to the normal Unikraft `uk_sys_openat(...)` path

This is how intercept becomes a first-class front end while still preserving
the old local behavior when disabled.

### 13.2 `close()` hook in `posix-fdtab/fdtab.c`

The `close()` syscall implementation now does:

1. call `uk_intercept_close(fd)`
2. if the result is anything except `-ENOTSUP`, return it
3. otherwise fall back to `uk_sys_close(fd)`

This is where remote fd ownership matters:

- remote fds are closed on the server
- local fds are closed through the normal guest fd table

### 13.3 `read()` hook in `posix-fdio/fd-shim.c`

The `read()` syscall implementation now does:

1. call `uk_intercept_read(fd, buf, count)`
2. if the result is anything except `-ENOTSUP`, return it
3. otherwise continue with the existing `uk_fdtab_shim_get(...)` path

This is important because remote-only fds do not live in Unikraft's local
`uk_ofile` / shim structures. Without this hook, those remote fds would just
look like `EBADF`.

### 13.4 `write()` hook in `posix-fdio/fd-shim.c`

The same pattern is now used for `write()`:

1. call `uk_intercept_write(fd, buf, count)`
2. if not `-ENOTSUP`, return it
3. otherwise use the original local shim path

### 13.5 What was not changed

At the moment:

- `writev()` is still left on the normal local path
- `readv()` is still left on the normal local path
- the intercept library does not yet own all fd-producing syscalls

So the staged model still relies on examples only using the implemented remote
syscall subset.

## 14. How File Descriptors Are Managed

This is the part that usually causes confusion, so it is worth stating
carefully.

### 14.1 There are two fd worlds

Current code effectively has two fd domains:

1. local Unikraft fds
   - backed by the guest's own fd table/shim structures
2. remote intercept fds
   - numbers returned by the server
   - tracked only by the intercept bitmap

The current design does not merge those two domains.

### 14.2 How a remote fd is created

Current path:

1. guest calls intercepted `openat()`
2. `uk_intercept_openat()` sends a remote RPC
3. server opens the host file and creates its own client-fd mapping
4. server returns the mapped client fd
5. guest marks that fd number in `intercept_remote_fds[]`

After that, that fd is considered remote-owned by the guest intercept layer.

### 14.3 How a remote fd is consumed

For `read()`, `write()`, and `close()`:

- intercept first checks the bitmap
- if the bit is set, the operation goes to the server
- if the bit is not set, the wrapper returns `-ENOTSUP`
- caller then falls back to normal local Unikraft behavior

### 14.4 How a remote fd is destroyed

`uk_intercept_close()`:

1. verifies the fd is remote
2. performs remote close RPC
3. clears the fd bit in the bitmap

After that, the guest no longer treats that fd as remote.

### 14.5 Why this is not a full fd integration

The remote bitmap only tracks ownership.

It does not:

- allocate guest-local `uk_ofile` structures
- install remote entries into Unikraft's fd table
- participate in `dup()`
- participate in `poll()`
- participate in `readv()` / `writev()` yet

That is intentional for the current staged development model.

### 14.6 What happens if you use a non-implemented syscall on a remote fd

Usually one of two things happens:

- if the syscall path was hooked into intercept and recognizes remote fds, it
  may route correctly
- if the syscall path was not hooked, the fd will likely fail local resolution
  and become `-EBADF`

That is why the test-app guidance is strict:

- only use the subset of syscalls that has been explicitly implemented in the
  remote model

## 15. Current Example Workflow

The sample app in `c-intercept/intercept.c` now demonstrates:

1. `access("/tmp", F_OK)`
2. `openat(AT_FDCWD, "/tmp/aaa", O_CREAT | O_TRUNC | O_WRONLY, 0644)`
3. `write(fd, "written from example", ...)`
4. `close(fd)`
5. `openat(AT_FDCWD, "/tmp/aaa", O_RDONLY, 0)`
6. `read(fd, ...)`
7. `close(fd)`

This is a better example than the old read-only one because it shows:

- remote path opening
- remote fd creation
- remote write request encoding
- remote close
- remote reopen
- remote read with variable-length payload
- remote close again

It is the first end-to-end demonstration of the current remote-only fd model.

## 16. Error Model

The code consistently uses negative errno returns internally.

At the guest-side intercept layer:

- success: `>= 0` or positive byte count
- failure: `-errno`

The wrappers preserve caller-visible `errno` on success.

This matters because:

- transport/RPC code may touch `errno`
- the guest syscall caller should see normal Linux-style behavior after a
  successful intercepted syscall

On remote syscall failure:

- the server returns `result = -1` and `err = <errno>`
- the guest leaf converts that to `-err`

On malformed protocol data:

- the guest returns protocol errors such as `-EPROTO`, `-EINVAL`, or
  `-EMSGSIZE`

## 17. Current Limits and Assumptions

The current implementation is intentionally limited.

### 17.1 Single-flight RPC model

The code currently assumes:

- one synchronous request/reply exchange at a time
- one connected socket reused globally

There is no concurrency control around:

- `rpc_xid`
- transport send/recv ownership

### 17.2 Fixed stack buffers

The shared RPC core uses fixed-size stack buffers for:

- request
- reply

Current size:

- `UK_INTERCEPT_RPC_BUF_SIZE = 8192`

This is fine for current small bring-up syscalls, but it is a real limit.

### 17.3 Single-fragment replies only

The RPC core rejects multi-fragment ONC RPC replies.

That means large replies are not supported in the current model unless they fit
in one fragment and one reply buffer.

### 17.4 Remote-only fd usage is still scoped

The code is not trying to support arbitrary mixing of:

- remote file fds
- local file fds
- local sockets/pipes/etc.

Current examples and intended use rely on only using implemented remote
syscalls in the test app.

## 18. How to Extend the Current Model

If you add another syscall now, the intended workflow is:

1. add a new RPC leaf file:
   - `rpc_<syscall>.c`
2. define:
   - request struct
   - response struct
   - encode callback
   - decode callback
   - `uk_intercept_rpc_<syscall>()`
3. add a prototype in `intercept_internal.h`
4. if needed, add a policy wrapper in `intercept.c`
5. hook the relevant Unikraft syscall path so it consults the intercept wrapper
6. if fd ownership matters, keep using the remote bitmap in `intercept.c`
7. record the new behavior in `AGENTS.md`

The shared files should change only if the protocol needs a new common feature.

That is the main reason the split refactor was worth doing.

## 19. Summary

The current intercept library has three clear layers:

1. policy layer in `intercept.c`
   - readiness
   - fd ownership
   - syscall admission rules
   - errno preservation

2. transport layer in `transport.c`
   - TCP connect/send/recv
   - no syscall knowledge

3. RPC layer in `rpc/`
   - shared XDR helpers
   - shared ONC RPC framing
   - one leaf file per syscall

And it relies on explicit hook points in other Unikraft libraries:

- `posix-vfs` for `openat()`
- `posix-fdtab` for `close()`
- `posix-fdio` for `read()` and `write()`

The current fd model is intentionally minimal:

- remote fds are tracked by a bitmap
- only tracked remote fds are intercepted for fd-consuming syscalls
- local fallback remains available when intercept is disabled or not applicable

This is enough to support the current staged examples cleanly, while keeping
the code structured well enough for the next syscalls.
