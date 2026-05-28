# intercept-probe

## Goal

`intercept-probe` is the smallest active sample in the current schema. It
validates only the path-based intercept path and the guest-to-host RPC reachability.

## What It Exercises Today

- `access("/tmp", F_OK)` for an existing host path
- `access("/tmp/intercept-probe-missing", F_OK)` for a missing host path

## What Unikraft Already Has

The current intercept implementation already has everything this sample needs:

- `access()`
- TCP transport to the syscall server
- fallback behavior for unsupported syscalls

## Why This Sample Stays Small

This sample should not grow into a general filesystem regression test. If path
probes work but fd-based flows fail, the next sample to inspect is
`intercept-dirfd` or `intercept-rw`, not `intercept-probe`.
