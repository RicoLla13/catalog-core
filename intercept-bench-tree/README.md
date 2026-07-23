# intercept-bench-tree

`intercept-bench-tree` is the workflow benchmark for the client-side
intercept project.

Its end-to-end timing is the primary workflow result. Future syscall/RPC
attribution should explain this result without replacing it.

It measures an application-like read-only tree traversal in three modes:

1. host OS native
2. Unikraft without intercept, backed by QEMU `9pfs`
3. Unikraft with intercept enabled

## Workload

Each timed iteration performs:

- `access()` on the tree root
- `stat()` on the manifest
- `openat()` on the root directory
- `open()` and `read()` of the manifest
- relative `fstatat()` and `openat()` for each manifest entry
- one `pread()` with offset-stability validation
- `read()` for the remaining files
- `stat()` and `lstat()` on a symlink
- expected negative `ENOENT` and `ENOTDIR` checks
- `close()` of opened descriptors

The app prints one machine-readable row:

```text
BENCH name=tree_traversal iterations=50 total_ns=... avg_ns=... p50_ns=... p95_ns=... min_ns=... max_ns=... ops_per_sec=... mib_per_sec=0.00
```

## Run

Host native:

```sh
./run-host.sh
```

Guest without intercept:

```sh
./setup.sh
./run-guest-local.sh
```

Guest with intercept:

```sh
./setup.sh
./run-guest-intercept.sh
```

To capture comparable results:

```sh
./capture-results.sh host
./capture-results.sh local
./capture-results.sh intercept
./capture-results.sh all
```

Each capture run creates `results/<timestamp>/` with logs, CSV files, metadata,
and `all.csv` when using `all`.
