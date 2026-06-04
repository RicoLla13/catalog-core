#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define OK_PREFIX "[]"
#define ERR_PREFIX "[x]"

static int expect_regular_stat(const char *label, const struct stat *st)
{
	if (!S_ISREG(st->st_mode)) {
		dprintf(1, ERR_PREFIX " %s returned non-regular mode=%o\n",
			label, st->st_mode);
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char buf[128];
	const char *root_path = "/tmp/intercept-pathmeta-root";
	const char *existing_path = "/tmp/intercept-pathmeta-root/existing.txt";
	const char *link_path = "/tmp/intercept-pathmeta-root/existing-link.txt";
	const char *runtime_path = "/tmp/intercept-pathmeta-root/runtime.txt";
	const char *missing_path = "/tmp/intercept-pathmeta-root/missing.txt";
	const char *payload = "runtime data written through open()";
	int fd = -1;
	int rc;
	int failed = 0;
	off_t pos;
	struct stat st;
	struct stat link_st;
	ssize_t nread;
	ssize_t nwritten;

	(void) argc;
	(void) argv;

	errno = 0;
	rc = access(root_path, F_OK);
	if (rc == 0) {
		dprintf(1, OK_PREFIX " access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n",
			root_path, rc, errno);
	} else {
		dprintf(1, ERR_PREFIX " access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
			root_path, rc, errno, strerror(errno));
		return 1;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = stat(existing_path, &st);
	if (rc == 0 && !expect_regular_stat("stat(existing)", &st)) {
		dprintf(1,
			OK_PREFIX " stat(\"%s\") succeeded: rc=%d errno=%d mode=%o size=%lld\n",
			existing_path, rc, errno, st.st_mode, (long long) st.st_size);
	} else {
		dprintf(1,
			ERR_PREFIX " stat(\"%s\") failed: rc=%d errno=%d (%s)\n",
			existing_path, rc, errno, strerror(errno));
		return 1;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = lstat(existing_path, &st);
	if (rc == 0 && !expect_regular_stat("lstat(existing)", &st)) {
		dprintf(1,
			OK_PREFIX " lstat(\"%s\") on regular file succeeded: rc=%d errno=%d mode=%o size=%lld\n",
			existing_path, rc, errno, st.st_mode, (long long) st.st_size);
	} else {
		dprintf(1,
			ERR_PREFIX " lstat(\"%s\") failed: rc=%d errno=%d (%s)\n",
			existing_path, rc, errno, strerror(errno));
		return 1;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = stat(link_path, &st);
	if (rc == 0 && !expect_regular_stat("stat(link)", &st)) {
		dprintf(1,
			OK_PREFIX " stat(\"%s\") followed the symlink: rc=%d errno=%d mode=%o size=%lld\n",
			link_path, rc, errno, st.st_mode, (long long) st.st_size);
	} else {
		dprintf(1,
			ERR_PREFIX " stat(\"%s\") failed: rc=%d errno=%d (%s)\n",
			link_path, rc, errno, strerror(errno));
		return 1;
	}

	memset(&link_st, 0, sizeof(link_st));
	errno = 0;
	rc = lstat(link_path, &link_st);
	if (rc == 0 && S_ISLNK(link_st.st_mode)) {
		dprintf(1,
			OK_PREFIX " lstat(\"%s\") preserved symlink metadata: rc=%d errno=%d mode=%o size=%lld\n",
			link_path, rc, errno, link_st.st_mode,
			(long long) link_st.st_size);
	} else {
		dprintf(1,
			ERR_PREFIX " lstat(\"%s\") failed or did not return a symlink: rc=%d errno=%d mode=%o (%s)\n",
			link_path, rc, errno, link_st.st_mode, strerror(errno));
		return 1;
	}

	errno = 0;
	fd = open(runtime_path, O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (fd >= 0) {
		dprintf(1,
			OK_PREFIX " open(\"%s\", O_CREAT|O_TRUNC|O_RDWR, 0644) succeeded: fd=%d errno=%d\n",
			runtime_path, fd, errno);
	} else {
		dprintf(1,
			ERR_PREFIX " open(\"%s\", O_CREAT|O_TRUNC|O_RDWR, 0644) failed: fd=%d errno=%d (%s)\n",
			runtime_path, fd, errno, strerror(errno));
		return 1;
	}

	errno = 0;
	nwritten = write(fd, payload, strlen(payload));
	if (nwritten >= 0) {
		dprintf(1, OK_PREFIX " write(%d, %zu) succeeded: rc=%zd errno=%d\n",
			fd, strlen(payload), nwritten, errno);
	} else {
		dprintf(1, ERR_PREFIX " write(%d, %zu) failed: rc=%zd errno=%d (%s)\n",
			fd, strlen(payload), nwritten, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = stat(runtime_path, &st);
	if (rc == 0 && st.st_size == (off_t) strlen(payload)) {
		dprintf(1,
			OK_PREFIX " stat(\"%s\") after write succeeded: rc=%d errno=%d size=%lld\n",
			runtime_path, rc, errno, (long long) st.st_size);
	} else {
		dprintf(1,
			ERR_PREFIX " stat(\"%s\") after write failed or returned unexpected size=%lld errno=%d (%s)\n",
			runtime_path, (long long) st.st_size, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstat(fd, &st);
	if (rc == 0 && st.st_size == (off_t) strlen(payload)) {
		dprintf(1,
			OK_PREFIX " fstat(%d) matched path metadata: rc=%d errno=%d size=%lld\n",
			fd, rc, errno, (long long) st.st_size);
	} else {
		dprintf(1,
			ERR_PREFIX " fstat(%d) failed or returned unexpected size=%lld errno=%d (%s)\n",
			fd, (long long) st.st_size, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	pos = lseek(fd, 0, SEEK_SET);
	if (pos >= 0) {
		dprintf(1, OK_PREFIX " lseek(%d, 0, SEEK_SET) succeeded: off=%lld errno=%d\n",
			fd, (long long) pos, errno);
	} else {
		dprintf(1, ERR_PREFIX " lseek(%d, 0, SEEK_SET) failed: off=%lld errno=%d (%s)\n",
			fd, (long long) pos, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(buf, 0, sizeof(buf));
	errno = 0;
	nread = read(fd, buf, sizeof(buf) - 1);
	if (nread >= 0) {
		buf[nread] = '\0';
		dprintf(1,
			OK_PREFIX " read(%d, %zu) succeeded: rc=%zd errno=%d data=\"%s\"\n",
			fd, sizeof(buf) - 1, nread, errno, buf);
	} else {
		dprintf(1, ERR_PREFIX " read(%d, %zu) failed: rc=%zd errno=%d (%s)\n",
			fd, sizeof(buf) - 1, nread, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = stat(missing_path, &st);
	if (rc == -1 && errno == ENOENT) {
		dprintf(1,
			OK_PREFIX " stat(\"%s\") missing as expected: rc=%d errno=%d (%s)\n",
			missing_path, rc, errno, strerror(errno));
	} else {
		dprintf(1,
			ERR_PREFIX " stat(\"%s\") returned unexpected result: rc=%d errno=%d (%s)\n",
			missing_path, rc, errno, strerror(errno));
		failed = 1;
	}

out:
	if (fd >= 0) {
		errno = 0;
		rc = close(fd);
		if (rc == 0) {
			dprintf(1, OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n",
				fd, rc, errno);
		} else {
			dprintf(1, ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n",
				fd, rc, errno, strerror(errno));
			failed = 1;
		}
	}

	return failed;
}
