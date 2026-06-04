# intercept-sqlite-ro

`intercept-sqlite-ro` is an exploratory read-only SQLite probe over the host
syscall-server bridge.

## Purpose

It answers a narrow question:

- can the current intercept model open a host-backed SQLite database read-only
- can SQLite run enough metadata, locking, and positioned-read traffic to
  execute a simple `SELECT`

This sample intentionally does not claim full SQLite support. It is the first
probe before any read-write, journal, or WAL work.

## Host Fixture

`run.sh` seeds:

- `/tmp/intercept-sqlite-ro-root/chinook.db`

by copying the existing catalog database from:

- `sqlite/rootfs/chinook.db`

## Run

1. Start the host syscall server:

   ```sh
   cd repos/syscall-server
   make create_folders server client test_suite
   ./build/syscall_server
   ```

2. Configure the guest once:

   ```sh
   ./setup.sh
   make menuconfig
   ```

3. Launch the sample:

   ```sh
   ./run.sh
   ```

## Expected Checks

- preflight `access()`, `stat()`, and `open()` succeed on the seeded database
- `sqlite3_open_v2(..., SQLITE_OPEN_READONLY, ...)` succeeds
- a schema query and a small `Album` query return rows

If it fails, the stderr-style output should show whether the first blocker is
in preflight path handling, SQLite open, statement prepare, or stepping rows.
