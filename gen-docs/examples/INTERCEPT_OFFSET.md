# intercept-offset

## Goal

`intercept-offset` is the positioned-read sample for tracked remote file
descriptors.

## What It Exercises Today

- `access()` on `/tmp`
- absolute-path `openat()` with create/truncate on one tracked remote file fd
- `write()` to seed the remote file content
- baseline `lseek(fd, 0, SEEK_CUR)`
- `pread64()` from multiple offsets on the same tracked remote fd
- post-`pread64()` `lseek(fd, 0, SEEK_CUR)` to prove offset stability
- `close()` on the tracked remote file fd

## Why It Exists Separately

`pread64()` is not just "another read". Its contract is that positioned reads
must not mutate the fd's current offset. That is a distinct regression target
from the sequential `read()` and `lseek()` flow in `intercept-rw`.

## Current Boundary

This sample is the right place for offset-stable read behavior. If future work
adds `pwrite64()` or richer offset semantics, extend this sample or add a
neighboring offset-focused sample instead of growing `intercept-rw`.

## Current Limitation

The current guest/server wire contract still limits `pread64()` offsets:

- the existing syscall-server protocol defines `pread.offset` as XDR `long`
- the guest therefore currently sends only offsets representable in signed
  32-bit range
- this sample is still valid for proving offset stability, but it does not yet
  validate true large-offset `pread64()` behavior
