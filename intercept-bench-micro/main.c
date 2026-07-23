#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_FILE_PATH "/tmp/intercept-bench-micro-root/existing.txt"
#define PAYLOAD_SIZE (1024 * 1024)
#define ACCESS_ITERS 500U
#define STAT_ITERS 500U
#define OPEN_CLOSE_ITERS 200U
#define READ_ITERS 128U
#define PREAD_SMALL_ITERS 500U
#define PREAD_MEDIUM_ITERS 128U
#define MAX_SAMPLES ACCESS_ITERS

struct read_ctx {
	int fd;
	size_t size;
	off_t offset;
};

static int cmp_u64(const void *lhs, const void *rhs)
{
	const uint64_t a = *(const uint64_t *) lhs;
	const uint64_t b = *(const uint64_t *) rhs;

	if (a < b)
		return -1;
	if (a > b)
		return 1;
	return 0;
}

static uint64_t now_ns(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		fprintf(stderr, "clock_gettime failed: errno=%d (%s)\n", errno,
			strerror(errno));
		exit(1);
	}

	return (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
}

static size_t percentile_index(size_t count, size_t pct)
{
	size_t idx;

	assert(count > 0);
	assert(pct > 0);
	assert(pct <= 100);
	idx = (count * pct + 99U) / 100U;
	if (idx == 0)
		return 0;
	return idx - 1U;
}

static void self_check_percentiles(void)
{
	uint64_t sample[] = { 5, 1, 10, 3, 8 };

	qsort(sample, sizeof(sample) / sizeof(sample[0]), sizeof(sample[0]), cmp_u64);
	assert(sample[percentile_index(5, 50)] == 5);
	assert(sample[percentile_index(5, 95)] == 10);
}

static int bench_access_existing(void *ctx)
{
	const char *path = ctx;

	return access(path, F_OK);
}

static int bench_stat_existing(void *ctx)
{
	struct stat st;
	const char *path = ctx;

	return stat(path, &st);
}

static int bench_open_close_existing(void *ctx)
{
	int fd;
	const char *path = ctx;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	return close(fd);
}

static int bench_read_seq(void *ctx)
{
	char buf[4096];
	struct read_ctx *read_ctx = ctx;
	ssize_t nread;

	nread = read(read_ctx->fd, buf, read_ctx->size);
	if (nread != (ssize_t) read_ctx->size)
		return -1;

	read_ctx->offset += (off_t) nread;
	return 0;
}

static int bench_pread_fixed(void *ctx)
{
	char buf[16384];
	struct read_ctx *read_ctx = ctx;
	ssize_t nread;

	nread = pread(read_ctx->fd, buf, read_ctx->size, read_ctx->offset);
	if (nread != (ssize_t) read_ctx->size)
		return -1;

	read_ctx->offset += (off_t) read_ctx->size;
	if ((size_t) read_ctx->offset + read_ctx->size > PAYLOAD_SIZE)
		read_ctx->offset = 0;
	return 0;
}

static void run_benchmark(const char *name, size_t iterations,
	int (*op)(void *), void *ctx, size_t bytes_per_iter)
{
	uint64_t samples[MAX_SAMPLES];
	uint64_t start_ns;
	uint64_t stop_ns;
	uint64_t total_ns = 0;
	uint64_t min_ns = UINT64_MAX;
	uint64_t max_ns = 0;
	uint64_t p50_ns;
	uint64_t p95_ns;
	double avg_ns;
	double ops_per_sec;
	double mib_per_sec = 0.0;
	size_t i;

	assert(iterations <= MAX_SAMPLES);

	for (i = 0; i < iterations; ++i) {
		errno = 0;
		start_ns = now_ns();
		if (op(ctx) != 0) {
			fprintf(stderr, "benchmark '%s' failed on iteration %zu: errno=%d\n",
				name, i, errno);
			exit(1);
		}
		stop_ns = now_ns();
		samples[i] = stop_ns - start_ns;
		total_ns += samples[i];
		if (samples[i] < min_ns)
			min_ns = samples[i];
		if (samples[i] > max_ns)
			max_ns = samples[i];
	}

	qsort(samples, iterations, sizeof(*samples), cmp_u64);
	p50_ns = samples[percentile_index(iterations, 50)];
	p95_ns = samples[percentile_index(iterations, 95)];
	avg_ns = (double) total_ns / (double) iterations;
	ops_per_sec = ((double) iterations * 1000000000.0) / (double) total_ns;
	if (bytes_per_iter > 0)
		mib_per_sec = ((double) iterations * (double) bytes_per_iter *
			1000000000.0) / ((double) total_ns * 1024.0 * 1024.0);

	printf(
		"BENCH name=%s iterations=%zu total_ns=%" PRIu64 " avg_ns=%.2f p50_ns=%" PRIu64 " p95_ns=%" PRIu64 " min_ns=%" PRIu64 " max_ns=%" PRIu64 " ops_per_sec=%.2f mib_per_sec=%.2f\n",
		name, iterations, total_ns, avg_ns, p50_ns, p95_ns, min_ns,
		max_ns, ops_per_sec, mib_per_sec);
	fflush(stdout);
}

static int open_for_read(const char *path)
{
	int fd = open(path, O_RDONLY);

	if (fd < 0) {
		perror("open benchmark file");
		exit(1);
	}
	return fd;
}

static int run_read_benchmark(const char *name, const char *path,
	size_t iterations, size_t size, int (*op)(void *))
{
	struct read_ctx read_ctx;

	read_ctx.fd = open_for_read(path);
	read_ctx.size = size;
	read_ctx.offset = 0;
	run_benchmark(name, iterations, op, &read_ctx, size);
	if (close(read_ctx.fd) != 0) {
		perror("close benchmark file");
		return 1;
	}
	return 0;
}

static void require_fixture(const char *path)
{
	struct stat st;

	if (stat(path, &st) != 0) {
		fprintf(stderr, "fixture missing at %s: errno=%d (%s)\n", path,
			errno, strerror(errno));
		exit(1);
	}

	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "fixture is not a regular file: %s\n", path);
		exit(1);
	}

	if (st.st_size < PAYLOAD_SIZE) {
		fprintf(stderr, "fixture too small: path=%s size=%lld expected>=%d\n",
			path, (long long) st.st_size, PAYLOAD_SIZE);
		exit(1);
	}
}

static int run_smoke_probe(const char *path)
{
	int rc;

	errno = 0;
	rc = access(path, F_OK);
	if (rc != 0) {
		fprintf(stderr, "SMOKE access path=%s rc=%d errno=%d (%s)\n",
			path, rc, errno, strerror(errno));
		return 1;
	}
	fprintf(stderr, "SMOKE access path=%s rc=%d errno=%d\n", path, rc, errno);

	errno = 0;
	rc = stat(path, &(struct stat) { 0 });
	if (rc != 0) {
		fprintf(stderr, "SMOKE stat path=%s rc=%d errno=%d (%s)\n",
			path, rc, errno, strerror(errno));
		return 1;
	}
	fprintf(stderr, "SMOKE stat path=%s rc=%d errno=%d\n", path, rc, errno);

	errno = 0;
	rc = close(open(path, O_RDONLY));
	if (rc != 0) {
		fprintf(stderr, "SMOKE open_close path=%s rc=%d errno=%d (%s)\n",
			path, rc, errno, strerror(errno));
		return 1;
	}
	fprintf(stderr, "SMOKE open_close path=%s rc=%d errno=%d\n", path, rc,
		errno);

	return 0;
}

int main(int argc, char **argv)
{
	const char *path = DEFAULT_FILE_PATH;
	int smoke_only = 0;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	if (argc > 1)
		path = argv[1];
	if (argc > 2 && strcmp(argv[2], "--smoke") == 0)
		smoke_only = 1;

	self_check_percentiles();
	if (smoke_only) {
		if (run_smoke_probe(path) != 0)
			return 1;
	} else {
		require_fixture(path);
	}

	run_benchmark("access_existing", ACCESS_ITERS, bench_access_existing,
		(void *) path, 0);
	run_benchmark("stat_existing", STAT_ITERS, bench_stat_existing,
		(void *) path, 0);
	run_benchmark("open_close_existing", OPEN_CLOSE_ITERS,
		bench_open_close_existing, (void *) path, 0);
	if (smoke_only)
		return 0;

	if (run_read_benchmark("read_seq_4k", path, READ_ITERS, 4096,
			bench_read_seq) != 0)
		return 1;
	if (run_read_benchmark("pread_64", path, PREAD_SMALL_ITERS, 64,
			bench_pread_fixed) != 0)
		return 1;
	if (run_read_benchmark("pread_4k", path, PREAD_MEDIUM_ITERS, 4096,
			bench_pread_fixed) != 0)
		return 1;
	return 0;
}
