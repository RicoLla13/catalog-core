# intercept-fs

## Goal

`intercept-fs` is the main remote-filesystem sample for the current staged
intercept design.

## What It Exercises Today

- `access()` on `/tmp`
- `openat()` on `/tmp` to obtain a tracked remote directory fd
- relative `openat(dirfd, "intercept-fs.txt", ...)`
- `write()` on a tracked remote file fd
- `fstatat(dirfd, "intercept-fs.txt", ...)`
- `fstat(fd, ...)`
- `read(fd, ...)`
- `close()` on both file and directory fds

## What Unikraft Already Has

The current intercept implementation already supports the whole sample flow:

- `access()`
- `openat()`
- `close()`
- `newfstatat()`
- `fstat()`
- `read()`
- `write()`

## What Is Still Missing Around This Story

The sample is deliberately scoped to what already works. Common filesystem
operations still missing from the intercept side include:

- `unlink()`
- `mkdir()`
- `rename()`
- `lseek()`
- `fcntl()`

Those are not required for the current sample, but they become important once
the remote filesystem workflow grows beyond a create/write/read demo.

## Observed Migration Issues

During the migration from the old bitmap model to the fd metadata table, two
real bugs were hit and fixed.

### 1. Guest fd vs remote fd namespace collision

Problem:

- the first fd-table version still reused the server-returned remote fd as the
  guest-visible fd
- this broke once intercept transport connected during boot, because the guest
  had already allocated a local socket fd for the transport

Observed failure mode:

- with `Require successful connect during boot`, the transport socket occupied
  guest-local fd `3`
- the first remote `openat()` also wanted to use remote/server fd `3`
- the remote fd collided with the local guest fd namespace

Fix:

- split guest-visible fd allocation from server-side remote fd identity
- store the server fd in the intercept table entry
- allocate a guest-visible remote fd only from slots that are free in both:
  - the intercept fd table
  - the normal local Unikraft fd table

### 2. `AT_FDCWD` misclassified as an error

Problem:

- the new dirfd resolver returns `AT_FDCWD` for absolute paths
- `AT_FDCWD` is `-100`
- the first caller-side check treated any negative resolved dirfd as an error

Observed failure mode:

- `openat(AT_FDCWD, "/tmp", ...)` returned `errno=100`
- the guest printed `Network is down`
- no `OPENAT` RPC reached the server

Fix:

- treat `AT_FDCWD` as a valid special dirfd, even though it is negative
- only return early for real negative errno values

### 3. Remote dirfd classification is still conservative

Problem:

- the current guest fd table records a backend kind, but it does not yet carry
  authoritative file type metadata from the server
- using `O_DIRECTORY` alone to decide whether a tracked remote fd may be used
  as a dirfd is too strict

Observed risk:

- a real remote directory opened without `O_DIRECTORY` can still be valid for
  later relative `openat()` / `newfstatat()`
- rejecting it locally would produce a guest-side false `ENOTDIR`

Current mitigation:

- the guest now lets the server remain the source of truth for dirfd validity
  on tracked remote fds

Future goal:

- store authoritative remote file type metadata in the guest fd table and
  re-tighten local dirfd checks once that metadata exists
