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

#define DEFAULT_ROOT_PATH "/tmp/intercept-bench-tree-root"
#define WORKFLOW_ITERS 50U
#define MAX_SAMPLES WORKFLOW_ITERS

struct tree_ctx {
	const char *root_path;
	char manifest_path[256];
	char link_path[256];
	char missing_path[256];
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

static void make_path(char *dst, size_t dst_size, const char *root,
	const char *name)
{
	int rc = snprintf(dst, dst_size, "%s/%s", root, name);

	if (rc < 0 || (size_t) rc >= dst_size) {
		fprintf(stderr, "path too long: root=%s name=%s\n", root, name);
		exit(1);
	}
}

static int read_one_file(int rootfd, const char *entry_name, int *checked_pread)
{
	char buf[96];
	struct stat st;
	off_t pos;
	off_t pos_after;
	ssize_t nread;
	int fd;

	if (fstatat(rootfd, entry_name, &st, 0) != 0)
		return -1;
	if (!S_ISREG(st.st_mode))
		return -1;

	fd = openat(rootfd, entry_name, O_RDONLY, 0);
	if (fd < 0)
		return -1;

	if (fstat(fd, &st) != 0) {
		(void) close(fd);
		return -1;
	}

	if (!*checked_pread) {
		pos = lseek(fd, 0, SEEK_CUR);
		if (pos < 0) {
			(void) close(fd);
			return -1;
		}

		nread = pread(fd, buf, 15, 0);
		if (nread != 15) {
			(void) close(fd);
			return -1;
		}

		pos_after = lseek(fd, 0, SEEK_CUR);
		if (pos_after != pos) {
			(void) close(fd);
			return -1;
		}

		*checked_pread = 1;
	} else {
		nread = read(fd, buf, sizeof(buf));
		if (nread <= 0) {
			(void) close(fd);
			return -1;
		}
	}

	return close(fd);
}

static int run_tree_workflow(void *arg)
{
	struct tree_ctx *ctx = arg;
	char manifest[256];
	char *entry;
	char *saveptr;
	struct stat st;
	ssize_t nread;
	int checked_pread = 0;
	int rootfd = -1;
	int manifestfd = -1;
	int leaf_fd = -1;
	int failed = 0;

	if (access(ctx->root_path, F_OK) != 0)
		return -1;
	if (stat(ctx->manifest_path, &st) != 0 || !S_ISREG(st.st_mode))
		return -1;

	rootfd = openat(AT_FDCWD, ctx->root_path, O_RDONLY | O_DIRECTORY, 0);
	if (rootfd < 0)
		return -1;

	manifestfd = open(ctx->manifest_path, O_RDONLY, 0);
	if (manifestfd < 0) {
		failed = 1;
		goto out;
	}

	memset(manifest, 0, sizeof(manifest));
	nread = read(manifestfd, manifest, sizeof(manifest) - 1);
	if (nread <= 0) {
		failed = 1;
		goto out;
	}
	manifest[nread] = '\0';

	if (close(manifestfd) != 0) {
		manifestfd = -1;
		failed = 1;
		goto out;
	}
	manifestfd = -1;

	for (entry = strtok_r(manifest, "\n", &saveptr);
	     entry;
	     entry = strtok_r(NULL, "\n", &saveptr)) {
		if (read_one_file(rootfd, entry, &checked_pread) != 0) {
			failed = 1;
			goto out;
		}
	}

	if (!checked_pread) {
		failed = 1;
		goto out;
	}

	if (stat(ctx->link_path, &st) != 0 || !S_ISREG(st.st_mode)) {
		failed = 1;
		goto out;
	}
	if (lstat(ctx->link_path, &st) != 0 || !S_ISLNK(st.st_mode)) {
		failed = 1;
		goto out;
	}

	errno = 0;
	if (stat(ctx->missing_path, &st) != -1 || errno != ENOENT) {
		failed = 1;
		goto out;
	}

	leaf_fd = openat(rootfd, "pages/index.txt", O_RDONLY, 0);
	if (leaf_fd < 0) {
		failed = 1;
		goto out;
	}

	errno = 0;
	if (fstatat(leaf_fd, "child-under-file", &st, 0) != -1 ||
	    errno != ENOTDIR) {
		failed = 1;
		goto out;
	}

out:
	if (leaf_fd >= 0 && close(leaf_fd) != 0)
		failed = 1;
	if (manifestfd >= 0 && close(manifestfd) != 0)
		failed = 1;
	if (rootfd >= 0 && close(rootfd) != 0)
		failed = 1;
	return failed ? -1 : 0;
}

static void run_benchmark(const char *name, size_t iterations,
	int (*op)(void *), void *ctx)
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
	size_t i;

	assert(iterations <= MAX_SAMPLES);

	for (i = 0; i < iterations; ++i) {
		errno = 0;
		start_ns = now_ns();
		if (op(ctx) != 0) {
			fprintf(stderr, "benchmark '%s' failed on iteration %zu: errno=%d (%s)\n",
				name, i, errno, strerror(errno));
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

	printf(
		"BENCH name=%s iterations=%zu total_ns=%" PRIu64 " avg_ns=%.2f p50_ns=%" PRIu64 " p95_ns=%" PRIu64 " min_ns=%" PRIu64 " max_ns=%" PRIu64 " ops_per_sec=%.2f mib_per_sec=0.00\n",
		name, iterations, total_ns, avg_ns, p50_ns, p95_ns, min_ns,
		max_ns, ops_per_sec);
	fflush(stdout);
}

int main(int argc, char **argv)
{
	struct tree_ctx ctx;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	ctx.root_path = argc > 1 ? argv[1] : DEFAULT_ROOT_PATH;
	make_path(ctx.manifest_path, sizeof(ctx.manifest_path), ctx.root_path,
		"manifest.txt");
	make_path(ctx.link_path, sizeof(ctx.link_path), ctx.root_path,
		"current-link.txt");
	make_path(ctx.missing_path, sizeof(ctx.missing_path), ctx.root_path,
		"missing.txt");

	run_benchmark("tree_traversal", WORKFLOW_ITERS, run_tree_workflow, &ctx);
	return 0;
}

