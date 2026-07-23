# intercept-bench-sqlite

The application benchmark tier for the client-side intercept project.

This is an end-to-end exploratory result. Future syscall/RPC attribution
belongs in the intercept layer rather than patched SQLite code.

It measures one fixed read-only SQLite workload in three modes:

1. host-native
2. Unikraft without intercept, backed by QEMU `9pfs`
3. Unikraft with intercept enabled

The app performs three unmeasured warmup queries, then repeats a measured
iteration 50 times. Each iteration opens `chinook.db`, runs `SELECT AlbumId,
Title FROM Album ORDER BY AlbumId LIMIT 5`, and closes the database. It emits
one machine-readable `BENCH` line with median and p95 timing.

Run `./run-host.sh`, `./run-guest-local.sh`, or `./run-guest-intercept.sh`.
Use `./capture-results.sh all` for CSV output. Capture defaults to five
independent launches per mode; set `BENCH_RUNS` to change that. The database
is read-only; this does not measure SQLite writes, journaling, WAL, or
persistence. The `sqlite_version` field in each log must be reported because
the host runner currently uses the system SQLite while the guest uses the
vendored Unikraft SQLite.
