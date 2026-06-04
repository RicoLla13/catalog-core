# intercept-sqlite-shell-ro

`intercept-sqlite-shell-ro` runs the SQLite shell against a host-seeded
database through the intercept bridge.

## Purpose

This is the interactive follow-on to `intercept-sqlite-ro`.

It keeps the database read-only and uses the stock `libsqlite` shell main so
you can probe real shell commands such as:

- `.tables`
- `.schema Album`
- `SELECT AlbumId, Title FROM Album LIMIT 5;`

## Host Fixture

`run.sh` seeds:

- `/tmp/intercept-sqlite-ro-root/chinook.db`

by copying:

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

3. Launch the shell sample:

   ```sh
   ./run.sh
   ```

The current `run.sh` passes a few starter commands automatically:

- `.tables`
- `SELECT name FROM sqlite_master WHERE type='table' LIMIT 8;`
- `SELECT AlbumId, Title FROM Album LIMIT 5;`

If those work, the next useful step is to edit `run.sh` and try more shell
commands against the same host-backed database path.
