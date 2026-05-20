# HTTP Server Roadmap

This document answers a specific question:

- what should change in the current architecture to support a `c-http`-style
  HTTP server cleanly?

## 1. Important Starting Point

For the exact `c-http/server.c` sample in this tree, you do not need additional
intercept syscalls just to let the guest HTTP server run.

Reason:

- its networking is local guest networking through Unikraft/lwIP
- its socket fds are local socket fds, not tracked remote file fds
- current intercept hooks for `read()`, `write()`, and `close()` already fall
  back to the local path for non-remote fds

So a simple "Hello from Unikraft" HTTP server is not blocked by the current
intercept architecture.

## 2. When Intercept Starts Mattering

Intercept becomes important when the HTTP server needs to serve content from
the remote syscall-server-backed filesystem, or when it uses richer userspace
I/O patterns.

Typical examples:

- open a file for a request path
- stat a file before serving it
- seek or pread within a file
- use vectored writes for headers plus body
- use nonblocking/event-driven fd control
- use poll/epoll on sockets and possibly other descriptors

## 3. Minimal Syscall Roadmap for Static File Serving

Current coverage already gives you:

- `openat()`
- `newfstatat()`
- `fstat()`
- `read()`
- `write()`
- `close()`

Next syscalls to add for practical file serving:

1. `lseek()`
2. `pread64()`
3. `writev()`
4. `fcntl()`

Why:

- `lseek()`
  - needed for stateful random-access reads
  - useful for resumable or offset-based serving

- `pread64()`
  - often better than `lseek() + read()` because it does not mutate shared file
    offset state
  - good fit for file-serving code paths

- `writev()`
  - many HTTP implementations send headers and body pieces with vectored writes
  - current intercept leaves it on the local path only

- `fcntl()`
  - often needed for nonblocking mode, close-on-exec flags, and common fd
    queries used by more realistic servers or libraries

## 4. Architectural Changes Beyond More Syscalls

The biggest future work is architectural, not just procedural.

### 4.1 Replace the bitmap-only remote fd model

Current state:

- one bitmap answers only "is this fd remote?"

For HTTP-server-style workloads, that will become too weak.

You should expect to grow toward:

- a remote descriptor metadata table
- per-entry descriptor kind:
  - regular file
  - directory
  - maybe later socket, pipe, etc.
- per-entry backend operations or backend tag
- room for flags, offset policy, and future poll/dup integration

### 4.2 Support mixed-backend cross-fd operations

The hard cases are not isolated syscalls. They are operations involving two fds
that may live in different backend worlds.

Examples:

- `sendfile(out_socket, in_file, ...)`
  - local socket + remote file
- `dup()` / `dup2()` / `dup3()`
  - preserve remote/backend identity
- `poll()` / `epoll()`
  - event model for local sockets and any future remote fd participation

This is where the architecture must move from:

- "subset of syscalls routed by bitmap"

to:

- "descriptor-aware backend dispatch"

### 4.3 Treat local sockets and remote files as a first-class mixed model

An HTTP server in this environment is likely to use:

- local guest sockets for networking
- remote bridged file descriptors for content

That combination is valid, but you should design for it explicitly:

- local sockets stay on the Unikraft/lwIP path
- remote regular files stay on the RPC path
- cross-backend helper syscalls need explicit behavior, not accidental fallback

## 5. Suggested Implementation Order

If your goal is "serve remote files over a local guest HTTP socket", the most
useful order is:

1. `lseek()`
2. `pread64()`
3. `writev()`
4. `fcntl()`
5. descriptor metadata table instead of bitmap-only tracking
6. `sendfile()` strategy for mixed local-socket / remote-file paths
7. `poll()` / `epoll()` only if the server design actually needs it

## 6. Practical Architecture Markers

If you want a checklist of concrete architecture changes to watch for, here it
is:

- add remote descriptor metadata, not just a bitmap
- keep backend dispatch explicit per fd
- define mixed-backend behavior for two-fd syscalls
- add vectored I/O support
- add random-access file read support
- add fd control support for realistic servers
- only add remote polling when there is a real consumer for it

## 7. Bottom Line

For the current `c-http` sample:

- no intercept architecture change is required just to run it

For a realistic HTTP server serving remote files:

- the next syscall work is `lseek()`, `pread64()`, `writev()`, `fcntl()`
- the next architecture work is a real remote descriptor table and explicit
  mixed-backend behavior
