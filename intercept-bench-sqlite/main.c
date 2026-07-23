#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_DB_PATH "/tmp/intercept-bench-sqlite-root/chinook.db"
#define WARMUP_ITERATIONS 3U
#define ITERATIONS 50U

static int cmp_u64(const void *lhs, const void *rhs)
{
	uint64_t a = *(const uint64_t *) lhs;
	uint64_t b = *(const uint64_t *) rhs;

	return (a > b) - (a < b);
}

static uint64_t now_ns(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
		exit(1);
	}
	return (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
}

static int run_query(const char *path)
{
	static const char *const sql =
		"SELECT AlbumId, Title FROM Album ORDER BY AlbumId LIMIT 5;";
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	int rows = 0;
	int rc;

	rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, NULL);
	if (rc != SQLITE_OK)
		goto fail;
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		goto fail;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		rows++;
	if (rc != SQLITE_DONE || rows != 5)
		goto fail;
	rc = sqlite3_finalize(stmt);
	stmt = NULL;
	if (rc != SQLITE_OK)
		goto fail;
	rc = sqlite3_close(db);
	db = NULL;
	return rc == SQLITE_OK ? 0 : -1;

fail:
	fprintf(stderr, "sqlite benchmark failed: rc=%d msg=%s\n", rc,
		db ? sqlite3_errmsg(db) : "no database handle");
	if (stmt)
		sqlite3_finalize(stmt);
	if (db)
		sqlite3_close(db);
	return -1;
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : DEFAULT_DB_PATH;
	uint64_t samples[ITERATIONS];
	uint64_t total_ns = 0;
	uint64_t start_ns;
	uint64_t stop_ns;
	uint64_t min_ns;
	uint64_t max_ns;
	size_t i;

	for (i = 0; i < WARMUP_ITERATIONS; ++i)
		if (run_query(path) != 0)
			return 1;

	for (i = 0; i < ITERATIONS; ++i) {
		start_ns = now_ns();
		if (run_query(path) != 0)
			return 1;
		stop_ns = now_ns();
		samples[i] = stop_ns - start_ns;
		total_ns += samples[i];
	}
	qsort(samples, ITERATIONS, sizeof(*samples), cmp_u64);
	min_ns = samples[0];
	max_ns = samples[ITERATIONS - 1];
	printf("BENCH name=sqlite_ro_query iterations=%u total_ns=%" PRIu64
		" avg_ns=%.2f p50_ns=%" PRIu64 " p95_ns=%" PRIu64
		" min_ns=%" PRIu64 " max_ns=%" PRIu64
		" ops_per_sec=%.2f mib_per_sec=0.00 sqlite_version=%s\n",
		ITERATIONS, total_ns, (double) total_ns / ITERATIONS,
		samples[24], samples[47], min_ns, max_ns,
		(double) ITERATIONS * 1000000000.0 / total_ns, sqlite3_libversion());
	return 0;
}
