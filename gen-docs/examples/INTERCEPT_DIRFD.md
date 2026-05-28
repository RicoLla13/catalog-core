# intercept-dirfd

## Goal

`intercept-dirfd` is the policy-focused sample for tracked remote directory
descriptors.

## What It Exercises Today

- `access()` on `/tmp`
- `openat()` on `/tmp` with `O_DIRECTORY`
- `openat()` on `/tmp` without `O_DIRECTORY`
- relative `openat(classified_dirfd, "intercept-dirfd.txt", ...)`
- `fstatat()` through both directory-fd forms
- local `ENOTDIR` rejection when a tracked remote regular file is reused as a
  dirfd

## Why It Exists Separately

This sample isolates descriptor classification and dirfd policy from the
broader read/write story. If it fails while `intercept-probe` passes, the
problem is likely in tracked remote fd classification or local dirfd checks.

## Current Boundary

This sample may create a remote file as part of its dirfd checks, but it is not
the place for broader sequential I/O, offset semantics, or future `pread64()`
tests. Those belong in `intercept-rw` or a future `intercept-offset` sample.
