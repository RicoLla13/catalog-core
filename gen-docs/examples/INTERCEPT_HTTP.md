# intercept-http

## Goal

`intercept-http` keeps the current sample close to `c-http` while making it an
intercept example:

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

1. `lseek()`
2. `pread64()`
3. `writev()`
4. `fcntl()`
5. remote descriptor metadata table instead of bitmap-only tracking

## Why The Descriptor Model Matters

The sample eventually needs to mix:

- local guest sockets for networking
- remote file descriptors for content

That means future work is not only about adding leaf RPC codecs. It also needs
descriptor-aware backend dispatch for mixed local/remote operations.
