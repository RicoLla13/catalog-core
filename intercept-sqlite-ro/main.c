#include <errno.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define OK_PREFIX "[ok]"
#define ERR_PREFIX "[x]"

static const char *const db_path = "/tmp/intercept-sqlite-ro-root/chinook.db";

static int run_query(sqlite3 *db, const char *label, const char *sql)
{
	sqlite3_stmt *stmt = NULL;
	int col_count;
	int rc;
	int row_count = 0;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		dprintf(1,
			ERR_PREFIX " sqlite3_prepare_v2(%s) failed: rc=%d ext=%d msg=%s sql=%s\n",
			label, rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db), sql);
		return 1;
	}

	col_count = sqlite3_column_count(stmt);
	dprintf(1, OK_PREFIX " sqlite3_prepare_v2(%s) succeeded: columns=%d\n",
		label, col_count);

	for (;;) {
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW) {
			int i;

			row_count++;
			dprintf(1, OK_PREFIX " %s row %d:", label, row_count);
			for (i = 0; i < col_count; i++) {
				const unsigned char *text = sqlite3_column_text(stmt, i);

				dprintf(1, " %s=%s",
					sqlite3_column_name(stmt, i),
					text ? (const char *) text : "NULL");
			}
			dprintf(1, "\n");
			continue;
		}

		if (rc == SQLITE_DONE) {
			dprintf(1, OK_PREFIX " sqlite3_step(%s) completed: rows=%d\n",
				label, row_count);
			break;
		}

		dprintf(1,
			ERR_PREFIX " sqlite3_step(%s) failed: rc=%d ext=%d msg=%s\n",
			label, rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db));
		(void) sqlite3_finalize(stmt);
		return 1;
	}

	rc = sqlite3_finalize(stmt);
	if (rc != SQLITE_OK) {
		dprintf(1,
			ERR_PREFIX " sqlite3_finalize(%s) failed: rc=%d ext=%d msg=%s\n",
			label, rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db));
		return 1;
	}

	dprintf(1, OK_PREFIX " sqlite3_finalize(%s) succeeded\n", label);
	return 0;
}

int main(int argc, char *argv[])
{
	sqlite3 *db = NULL;
	struct stat st;
	char probe[16];
	ssize_t nread;
	int fd = -1;
	int rc;
	int failed = 0;

	(void) argc;
	(void) argv;

	errno = 0;
	rc = access(db_path, R_OK);
	if (rc != 0) {
		dprintf(1, ERR_PREFIX " access(\"%s\", R_OK) failed: rc=%d errno=%d (%s)\n",
			db_path, rc, errno, strerror(errno));
		return 1;
	}
	dprintf(1, OK_PREFIX " access(\"%s\", R_OK) succeeded: rc=%d errno=%d\n",
		db_path, rc, errno);

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = stat(db_path, &st);
	if (rc != 0) {
		dprintf(1, ERR_PREFIX " stat(\"%s\") failed: rc=%d errno=%d (%s)\n",
			db_path, rc, errno, strerror(errno));
		return 1;
	}
	dprintf(1,
		OK_PREFIX " stat(\"%s\") succeeded: rc=%d errno=%d mode=%o size=%lld\n",
		db_path, rc, errno, st.st_mode, (long long) st.st_size);

	errno = 0;
	fd = open(db_path, O_RDONLY, 0);
	if (fd < 0) {
		dprintf(1, ERR_PREFIX " open(\"%s\", O_RDONLY) failed: fd=%d errno=%d (%s)\n",
			db_path, fd, errno, strerror(errno));
		return 1;
	}
	dprintf(1, OK_PREFIX " open(\"%s\", O_RDONLY) succeeded: fd=%d errno=%d\n",
		db_path, fd, errno);

	memset(probe, 0, sizeof(probe));
	errno = 0;
	nread = pread(fd, probe, sizeof(probe) - 1, 0);
	if (nread < 0) {
		dprintf(1, ERR_PREFIX " pread(%d, %zu, 0) failed: rc=%zd errno=%d (%s)\n",
			fd, sizeof(probe) - 1, nread, errno, strerror(errno));
		failed = 1;
		goto out;
	}
	probe[nread] = '\0';
	dprintf(1, OK_PREFIX " pread(%d, %zu, 0) succeeded: rc=%zd errno=%d data=\"%s\"\n",
		fd, sizeof(probe) - 1, nread, errno, probe);

	errno = 0;
	rc = close(fd);
	if (rc != 0) {
		dprintf(1, ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n",
			fd, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}
	dprintf(1, OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n",
		fd, rc, errno);
	fd = -1;

	rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READONLY, NULL);
	if (rc != SQLITE_OK) {
		dprintf(1,
			ERR_PREFIX " sqlite3_open_v2(\"%s\", SQLITE_OPEN_READONLY) failed: rc=%d ext=%d msg=%s\n",
			db_path, rc, db ? sqlite3_extended_errcode(db) : rc,
			db ? sqlite3_errmsg(db) : "no db handle");
		failed = 1;
		goto out;
	}
	dprintf(1, OK_PREFIX " sqlite3_open_v2(\"%s\") succeeded\n", db_path);

	if (run_query(db, "schema",
		      "SELECT name FROM sqlite_master "
		      "WHERE type = 'table' ORDER BY name LIMIT 8;") != 0) {
		failed = 1;
		goto out;
	}

	if (run_query(db, "album-sample",
		      "SELECT AlbumId, Title FROM Album "
		      "ORDER BY AlbumId LIMIT 5;") != 0) {
		failed = 1;
		goto out;
	}

out:
	if (db) {
		rc = sqlite3_close(db);
		if (rc != SQLITE_OK) {
			dprintf(1,
				ERR_PREFIX " sqlite3_close() failed: rc=%d ext=%d msg=%s\n",
				rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db));
			failed = 1;
		} else {
			dprintf(1, OK_PREFIX " sqlite3_close() succeeded\n");
		}
	}

	if (fd >= 0) {
		errno = 0;
		rc = close(fd);
		if (rc != 0) {
			dprintf(1, ERR_PREFIX " close(%d) failed during cleanup: rc=%d errno=%d (%s)\n",
				fd, rc, errno, strerror(errno));
			failed = 1;
		}
	}

	return failed ? 1 : 0;
}
