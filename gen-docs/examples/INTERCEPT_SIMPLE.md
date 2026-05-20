# intercept-simple

## Goal

`intercept-simple` is the smallest sample that proves the guest can reach the
host syscall server and resolve host paths through `access()`.

## What It Exercises Today

- `access("/tmp", F_OK)` for an existing host path
- `access("/tmp/intercept-simple-missing", F_OK)` for a missing host path

## What Unikraft Already Has

The current intercept implementation already has everything this sample needs:

- `access()`
- TCP transport to the syscall server
- fallback behavior for unsupported syscalls

## What Would Be Next

If this sample ever needs to go beyond path existence checks, the next step is
usually `openat()`. That is already implemented, so the natural upgrade path is
the richer `intercept-fs` sample rather than expanding `intercept-simple`.
