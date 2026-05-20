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
  - `read()`
  - `write()`
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

`c-intercept/intercept.c` demonstrates the current remote file subset:

1. remote path existence check
2. remote create/open
3. remote write
4. remote close
5. remote reopen
6. remote path metadata lookup
7. remote fd metadata lookup
8. remote read
9. remote close

The current sample is intentionally direct and prints with `dprintf()` instead
of buffered stdio.

## 4. What Is Not Hooked Yet

Notable gaps:

- `writev()`
- `lseek()`
- `fcntl()`
- `poll()` / `epoll()` integration for remote fds
- `dup()` / `dup2()` / `dup3()` for remote fds
- `sendfile()` across mixed local/remote backends

## 5. Recommended Next Syscalls

If the goal is broader real-program compatibility, the next priorities are:

1. `lseek()`
2. `fcntl()`
3. `pread64()`
4. `writev()`
5. `getcwd()`

That order is especially relevant once you move from `c-intercept` toward
HTTP-server-style workloads.

## 6. Extension Guidance

When adding another intercepted syscall:

1. add public wrapper in `include/uk/intercept.h`
2. add internal RPC entry point in `intercept_internal.h`
3. add or extend RPC constants in `rpc_internal.h`
4. implement the leaf codec in `rpc/rpc_<syscall>.c`
5. wire the syscall hook in the correct Unikraft subsystem
6. update `c-intercept` if the syscall belongs in the current sample story
7. update `gen-docs/`

If the syscall works on a remote fd, check whether the current bitmap model is
still enough. If it needs richer descriptor semantics, that is usually a sign
that the architecture should evolve, not just the syscall count.
