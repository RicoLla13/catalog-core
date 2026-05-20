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
