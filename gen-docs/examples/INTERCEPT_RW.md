# intercept-rw

## Goal

`intercept-rw` is the sequential remote file I/O sample for the current staged
intercept design.

## What It Exercises Today

- `access()` on `/tmp`
- absolute-path `openat()` with create/truncate on one tracked remote file fd
- repeated `write()` calls on that fd
- `lseek()` for current-offset, seek-forward, and rewind behavior
- `fstat()` on the tracked remote file fd
- `read()` after seek and after rewind
- `close()` on the tracked remote file fd

## Why It Exists Separately

This sample isolates one remote file descriptor and its sequential I/O
semantics. It does not depend on directory-fd policy beyond the initial
absolute-path open.

## Current Boundary

This sample is the right place for `lseek()` regressions today. Offset-stable
reads now live in the dedicated
`intercept-offset` sample rather than expanding this one indefinitely.
