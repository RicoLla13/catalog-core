# Intercept Hook Points

This document explains where the intercept layer is currently wired into
Unikraft and how the focused examples exercise those hook sites.

## 1. Hook Sites

Current hook sites:

- `repos/unikraft/lib/posix-vfs/syscalls.c`
  - `access()`
  - `openat()`
  - `newfstatat()`
- `repos/unikraft/lib/posix-fdtab/fdtab.c`
  - `close()`
- `repos/unikraft/lib/posix-fdio/fd-shim.c`
  - `fstat()`
  - `lseek()`
  - `pread64()`
  - `read()`
  - `write()`

## 2. Hook Contract

The common pattern is:

1. call intercept first
2. if intercept returns anything except `-ENOTSUP`, use that result
3. otherwise continue with the normal local Unikraft implementation

That pattern is what allows the same guest to mix:

- local sockets
- local files
- remote bridged filesystem syscalls

without intercept claiming every fd blindly.

## 3. Example-To-Hook Mapping

### 3.1 `intercept-probe`

Exercises:

- `access()`

Use this sample when you want to isolate:

- transport reachability
- RPC envelope validity
- path-only remote checks

### 3.2 `intercept-dirfd`

Exercises:

- `access()`
- `openat()`
- `newfstatat()`
- `close()`

Use this sample when you want to isolate:

- remote dirfd classification
- relative path policy
- local `ENOTDIR` rejection for tracked remote regular files

### 3.3 `intercept-rw`

Exercises:

- `access()`
- `openat()`
- `write()`
- `lseek()`
- `fstat()`
- `read()`
- `close()`

Use this sample when you want to isolate:

- sequential remote file I/O
- current-offset mutation through `lseek()`

### 3.4 `intercept-offset`

Exercises:

- `access()`
- `openat()`
- `write()`
- `lseek()`
- `pread64()`
- `close()`

Use this sample when you want to isolate:

- offset-stable positioned reads
- current `pread64()` protocol limitations

### 3.5 `intercept-http`

Exercises:

- intercepted `access()`
- normal local socket syscalls through lwIP

Use this sample when you want to isolate:

- mixed local-socket plus remote-filesystem behavior
- whether intercept correctly ignores non-remote fds

## 4. What Is Not Hooked Yet

Notable gaps:

- `writev()`
- `fcntl()`
- `dup()` / `dup2()` / `dup3()` for remote fds
- `poll()` / `epoll()` integration for remote fds
- `sendfile()` across mixed local/remote backends

## 5. Extension Checklist

When adding another intercepted syscall:

1. add the public wrapper in `include/uk/intercept.h`
2. add the internal RPC entry point in `intercept_internal.h`
3. add or extend RPC constants in `rpc_internal.h`
4. implement the leaf codec in `rpc/rpc_<syscall>.c`
5. wire the syscall hook in the correct Unikraft subsystem
6. add or update the focused example that owns that contract
7. update `gen-docs/`

If the syscall works on a remote fd, check whether the current descriptor-table
model is still sufficient. If it is not, that is usually a sign that the
architecture needs to evolve, not just the syscall count.
