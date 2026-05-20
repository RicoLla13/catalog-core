# Intercept Example Docs Index

This directory documents the current intercept sample set and the remaining
Unikraft-side work for each sample story.

Samples:

- [INTERCEPT_SIMPLE.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/examples/INTERCEPT_SIMPLE.md)
  - minimal remote path checks through `access()`

- [INTERCEPT_FS.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/examples/INTERCEPT_FS.md)
  - current remote filesystem workflow with directory fds, relative `openat()`,
    metadata, and read/write

- [INTERCEPT_HTTP.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/examples/INTERCEPT_HTTP.md)
  - mixed local-socket and remote-filesystem story
  - missing syscalls and descriptor-model work for serving remote files
