# intercept-http

## Goal

`intercept-http` is the integration sample in the current schema:

- the guest does one remote `access("/tmp", F_OK)` preflight through intercept
- the HTTP server itself still uses normal local guest sockets through lwIP

## What It Exercises Today

- intercepted `access()`
- local `socket()`, `bind()`, `listen()`, `accept()`, `read()`, `write()`,
  `close()` on guest socket fds

That mixed model already works with the current hook strategy because the
intercept path only claims supported remote filesystem calls and falls back on
non-remote fds.

## What Unikraft Already Has

The current implementation is enough for the fixed-response sample:

- `access()`
- remote-fd filtering for `read()`, `write()`, `close()`, `fstat()`
- local socket fallback for non-remote fds

## What Still Needs To Be Implemented for Remote File Serving

To evolve `intercept-http` from a fixed response into a server that opens and
serves files from the host-backed remote filesystem, the next Unikraft-side
work is:

1. `pread64()`
2. `writev()`
3. `fcntl()`
4. remote descriptor metadata table richer than file-vs-directory only
5. session/reset-safe remote fd lifetime semantics

## Why The Descriptor Model Matters

The sample eventually needs to mix:

- local guest sockets for networking
- remote file descriptors for content

That means future work is not only about adding leaf RPC codecs. It also needs
descriptor-aware backend dispatch for mixed local/remote operations.

## Schema Boundary

Do not turn `intercept-http` into the primary syscall regression sample. Keep
it focused on mixed local-socket and remote-filesystem integration. Detailed
syscall validation belongs in the focused examples.
