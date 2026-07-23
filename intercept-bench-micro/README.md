# intercept-bench-micro

`intercept-bench-micro` is the first benchmark app for the client-side
intercept project.

It runs the same syscall-shaped workload in three modes:

1. host OS native
2. Unikraft without intercept
3. Unikraft with intercept enabled

## Benchmark Matrix

| Workload | What it measures | Main stats |
| --- | --- | --- |
| `access_existing` | tiny metadata lookup cost | `avg_ns`, `p50_ns`, `p95_ns`, `ops_per_sec` |
| `stat_existing` | file metadata lookup cost | `avg_ns`, `p50_ns`, `p95_ns`, `ops_per_sec` |
| `open_close_existing` | open-path cost | `avg_ns`, `p50_ns`, `p95_ns`, `ops_per_sec` |
| `read_seq_4k` | sequential read behavior | `avg_ns`, `p50_ns`, `p95_ns`, `mib_per_sec` |
| `pread_64` | tiny positioned read overhead | `avg_ns`, `p50_ns`, `p95_ns`, `ops_per_sec` |
| `pread_4k` | medium positioned read behavior | `avg_ns`, `p50_ns`, `p95_ns`, `mib_per_sec` |

The benchmark fixture is prepared by scripts in advance:

- host native and intercept runs seed
  `/tmp/intercept-bench-micro-root/existing.txt`
- the local Unikraft baseline prepares `9pfs-rootfs/existing.txt`
  and mounts it through QEMU `virtio-9p`

The local baseline is therefore a guest-without-intercept baseline, not a
guest-ramfs baseline. That is the tradeoff that keeps the setup reproducible
without depending on the broken initrd-extract path in this tree.

The active benchmark set is also limited to what the current intercept RPC
path actually supports. In particular, larger single-call reads such as
`pread_16k` are intentionally excluded because the current guest RPC path uses
fixed 8 KiB request/reply buffers and returns `EMSGSIZE` above that ceiling.

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

The runner now only regenerates `.config` when the selected mode or its config
fragments change. Ordinary source edits use the normal incremental `make`
rebuild path.

To capture results into files instead of scraping the console manually:

```sh
./capture-results.sh host
./capture-results.sh local
./capture-results.sh intercept
./capture-results.sh all
```

Each capture run creates `results/<timestamp>/` with:

- `<mode>.log` raw output
- `<mode>.csv` parsed `BENCH` rows
- `<mode>.meta` small run metadata file
- `all.csv` when using `all`

## Output

Each benchmark prints one machine-readable line:

```text
BENCH name=... iterations=... total_ns=... avg_ns=... p50_ns=... p95_ns=... min_ns=... max_ns=... ops_per_sec=... mib_per_sec=...
```

That is enough to build the three-way comparison table for the report:

- host native
- Unikraft local via 9pfs
- Unikraft with intercept
