# Intercept RPC and Transport

This document explains the current transport and RPC implementation details.

## 1. RPC Constants

`repos/unikraft/lib/intercept/rpc/rpc_internal.h` centralizes three kinds of
constants.

Guest implementation limits:

- `UK_INTERCEPT_RPC_BUF_SIZE = 8192`
- `UK_INTERCEPT_MAX_PATH_LEN = 4096`

Generic ONC RPC constants:

- `RPC_LAST_FRAGMENT = 0x80000000U`
- `RPC_VERSION = 2U`
- `RPC_CALL = 0U`
- `RPC_REPLY = 1U`
- `RPC_MSG_ACCEPTED = 0U`
- `RPC_SUCCESS = 0U`
- `RPC_AUTH_NONE = 0U`

Syscall-server program and procedure IDs:

- `SYSCALL_PROG = 0x20000001U`
- `SYSCALL_VERS = 1U`
- `SYSCALL_OPENAT = 2U`
- `SYSCALL_CLOSE = 3U`
- `SYSCALL_READ = 4U`
- `SYSCALL_WRITE = 6U`
- `SYSCALL_NEWFSTATAT = 9U`
- `SYSCALL_FSTAT = 10U`
- `SYSCALL_LSEEK = 13U`
- `SYSCALL_ACCESS = 14U`

The `SYSCALL_*` values come from `repos/syscall-server/src/protocol/protocol.x`.

## 2. Transport

The transport keeps one TCP connection for the current single-flight RPC model.

State:

- one global socket fd
- one `connected` boolean

Current behavior:

- lazy TCP connect by default
- remote endpoint defaults to `172.44.0.1:9999`
- blocking send-all loop
- blocking receive-exact loop
- reset transport state after connect/send/recv failure or EOF

### 2.1 Boot-time probe

When eager boot transport policy is enabled, the guest does not just call raw
`connect()`. It performs an ONC RPC `NULLPROC` probe:

- request with no procedure payload
- accepted reply with no procedure payload

That is a stronger signal that the syscall server is actually reachable and
responding at the RPC layer.

## 3. XDR Helpers

Shared helpers in `rpc_xdr.c` currently cover:

- `u32` encode/decode
- `u64` encode/decode
- XDR opaque field encode/skip/decode
- bounded path length measurement via `strnlen()`

The `u64` decode helper exists because stat-style responses mix 32-bit XDR
fields with 64-bit XDR `hyper` fields such as:

- file size
- block count

## 4. Shared RPC Core

`rpc_core.c` provides the generic request/reply machinery.

Important pieces:

- global monotonically increasing `rpc_xid`
- generic ONC RPC call header emission
- accepted-reply parsing
- TCP record-marker handling
- full-consumption check for syscall-specific payloads

Current assumptions:

- one request in flight at a time
- one connected socket reused globally
- single-fragment replies only
- `AUTH_NONE` only

## 5. Per-Syscall RPC Leaves

### 5.1 `rpc_probe.c`

- wraps `NULLPROC`
- no procedure-specific request body
- no procedure-specific reply body
- used for eager boot server validation

### 5.2 `rpc_access.c`

- path plus mode
- `result` plus `err`

### 5.3 `rpc_openat.c`

- `dirfd`, `path`, `flags`, `mode`
- returns remote fd mapping from the server

### 5.4 `rpc_close.c`

- remote fd only

### 5.5 `rpc_newfstatat.c`

- `dirfd`, `path`, `flags`
- path-based metadata lookup
- decodes a mixed-width stat payload into guest `struct stat`

### 5.6 `rpc_fstat.c`

- remote fd only
- metadata lookup on an already-open tracked remote fd

### 5.7 `rpc_read.c`

- remote fd plus count
- variable-length reply payload

### 5.8 `rpc_write.c`

- remote fd plus variable-length request payload

### 5.9 `rpc_lseek.c`

- remote fd, 64-bit offset, and whence
- 64-bit result offset plus errno

## 6. Error Model

The intercept code uses negative errno returns internally.

Typical pattern:

- success: `>= 0`
- failure: `-errno`

Remote syscall failure is carried by syscall-specific reply payload fields:

- `result`
- `err`

The generic RPC layer only answers whether the transport and the RPC envelope
were valid.
