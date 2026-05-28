# Intercept Hook Points and Extension Notes

This document explains where the intercept layer is currently wired into
Unikraft and how the current sample flow exercises it.

## 1. Hook Points

Current hook sites:

- `repos/unikraft/lib/posix-vfs/syscalls.c`
  - `access()`
  - `openat()`
  - `newfstatat()`
- `repos/unikraft/lib/posix-fdtab/fdtab.c`
  - `close()`
- `repos/unikraft/lib/posix-fdio/fd-shim.c`
  - `pread64()`
  - `read()`
  - `write()`
  - `lseek()`
  - `fstat()`

## 2. Hook Behavior

The common pattern is:

1. call intercept first
2. if intercept returns anything except `-ENOTSUP`, use that result
3. otherwise continue with the normal local Unikraft implementation

That pattern is why the same guest can mix:

- local sockets
- local files
- remote bridged file syscalls

without intercept trying to claim everything.

## 3. Current Example Workflow

The current sample set is split by purpose:

1. `intercept-probe`
   - remote path existence checks with `access()`
2. `intercept-dirfd`
   - remote dirfd classification and validation
   - relative `openat()` / `fstatat()` policy
   - local `ENOTDIR` rejection for tracked remote regular files
3. `intercept-rw`
   - tracked remote file create/open
   - remote write
   - remote `lseek()`
   - remote `fstat()`
   - remote read
   - remote close
4. `intercept-offset`
   - tracked remote file write
   - remote `pread64()`
   - offset-stability checks against `SEEK_CUR`
5. `intercept-http`
   - remote `access()` preflight plus local guest HTTP sockets

The filesystem-focused sample remains intentionally direct and prints with
`dprintf()` instead of buffered stdio.

## 4. What Is Not Hooked Yet

Notable gaps:

- `writev()`
- `fcntl()`
- thread-safe RPC/fd-table serialization
- reconnect-safe remote fd lifetime semantics
- chunked remote I/O beyond the current fixed-size RPC buffers
- `poll()` / `epoll()` integration for remote fds
- `dup()` / `dup2()` / `dup3()` for remote fds
- `sendfile()` across mixed local/remote backends

## 5. Recommended Next Syscalls

If the goal is broader real-program compatibility, the next priorities are:

1. `fcntl()`
2. `writev()`
3. `getcwd()`
4. session/reset-safe remote fd lifetime semantics
5. richer descriptor sharing for future dup-style behavior

That order is especially relevant once you move from `intercept-rw` toward
remote-file-serving HTTP workloads.

## 6. Extension Guidance

When adding another intercepted syscall:

1. add public wrapper in `include/uk/intercept.h`
2. add internal RPC entry point in `intercept_internal.h`
3. add or extend RPC constants in `rpc_internal.h`
4. implement the leaf codec in `rpc/rpc_<syscall>.c`
5. wire the syscall hook in the correct Unikraft subsystem
6. update the relevant `intercept-*` sample if the syscall belongs in the
   current sample story
7. update `gen-docs/`

If the syscall works on a remote fd, check whether the current descriptor table
model is still enough. If it needs richer descriptor semantics, that is
usually a sign that the architecture should evolve, not just the syscall
count.

Priority note:

- tracked remote fds opened through intercept are now classified from remote
  `fstat()` metadata, so local remote-dirfd validation no longer depends on
  `O_DIRECTORY` heuristics alone

Current plan status:

1. `[done]` authoritative remote file-vs-directory tracking
2. `[done]` `lseek()`
3. `[done]` `pread64()`
4. `[pending]` `fcntl()`
5. `[pending]` session/reset contract for remote fd lifetime
6. `[pending]` `writev()`
