#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char buf[160];
	const char *dir_path = "/tmp";
	const char *relative_file = "intercept-fs.txt";
	const char *nested_missing = "intercept-fs-missing-child";
	const char *first = "written from intercept-fs";
	const char *second = " using relative openat";
	int dirfd = -1;
	int fd = -1;
	int rc;
	int failed = 0;
	struct stat st;
	off_t pos;
	ssize_t nread;
	ssize_t nwritten;

	(void) argc;
	(void) argv;

	/* Start with a simple path-based probe before switching to fd-based calls. */
	errno = 0;
	rc = access(dir_path, F_OK);
	if (rc == 0) {
		dprintf(1, "access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n",
			dir_path, rc, errno);
	} else {
		dprintf(1, "access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
			dir_path, rc, errno, strerror(errno));
		return 1;
	}

	/* Open the remote directory so the rest of the sample can use relative openat(). */
	errno = 0;
	dirfd = openat(AT_FDCWD, dir_path, O_RDONLY | O_DIRECTORY, 0);
	if (dirfd >= 0) {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_RDONLY|O_DIRECTORY) succeeded: fd=%d errno=%d\n",
			dir_path, dirfd, errno);
	} else {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_RDONLY|O_DIRECTORY) failed: fd=%d errno=%d (%s)\n",
			dir_path, dirfd, errno, strerror(errno));
		return 1;
	}

	/*
	 * Re-open the same directory without O_DIRECTORY. This should still be
	 * classified as a remote directory by post-open fstat metadata.
	 */
	errno = 0;
	fd = openat(AT_FDCWD, dir_path, O_RDONLY, 0);
	if (fd >= 0) {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_RDONLY) succeeded: fd=%d errno=%d\n",
			dir_path, fd, errno);
	} else {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_RDONLY) failed: fd=%d errno=%d (%s)\n",
			dir_path, fd, errno, strerror(errno));
		return 1;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstatat(fd, relative_file, &st, 0);
	if (rc == 0) {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) via non-O_DIRECTORY dirfd succeeded: rc=%d errno=%d mode=%o size=%lld nlink=%lu\n",
			fd, relative_file, rc, errno, st.st_mode,
			(long long) st.st_size, (unsigned long) st.st_nlink);
	} else {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) via non-O_DIRECTORY dirfd failed: rc=%d errno=%d (%s)\n",
			fd, relative_file, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	rc = close(fd);
	if (rc == 0) {
		dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n", fd, rc, errno);
		fd = -1;
	} else {
		dprintf(1, "close(%d) failed: rc=%d errno=%d (%s)\n", fd, rc,
			errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	fd = openat(dirfd, relative_file, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd >= 0) {
		dprintf(1,
			"openat(%d, \"%s\", O_CREAT|O_TRUNC|O_WRONLY, 0644) succeeded: fd=%d errno=%d\n",
			dirfd, relative_file, fd, errno);
	} else {
		dprintf(1,
			"openat(%d, \"%s\", O_CREAT|O_TRUNC|O_WRONLY, 0644) failed: fd=%d errno=%d (%s)\n",
			dirfd, relative_file, fd, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	/* Write the payload in two chunks to exercise repeated writes on one remote fd. */
	errno = 0;
	nwritten = write(fd, first, strlen(first));
	if (nwritten >= 0) {
		dprintf(1, "write(%d, %zu) succeeded: rc=%zd errno=%d\n", fd,
			strlen(first), nwritten, errno);
	} else {
		dprintf(1, "write(%d, %zu) failed: rc=%zd errno=%d (%s)\n", fd,
			strlen(first), nwritten, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	nwritten = write(fd, second, strlen(second));
	if (nwritten >= 0) {
		dprintf(1, "write(%d, %zu) succeeded: rc=%zd errno=%d\n", fd,
			strlen(second), nwritten, errno);
	} else {
		dprintf(1, "write(%d, %zu) failed: rc=%zd errno=%d (%s)\n", fd,
			strlen(second), nwritten, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	rc = close(fd);
	if (rc == 0) {
		dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n", fd, rc, errno);
		fd = -1;
	} else {
		dprintf(1, "close(%d) failed: rc=%d errno=%d (%s)\n", fd, rc,
			errno, strerror(errno));
		failed = 1;
		goto out;
	}

	/* Compare path-based metadata lookup with fd-based metadata lookup. */
	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstatat(dirfd, relative_file, &st, 0);
	if (rc == 0) {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) succeeded: rc=%d errno=%d mode=%o size=%lld nlink=%lu\n",
			dirfd, relative_file, rc, errno, st.st_mode,
			(long long) st.st_size, (unsigned long) st.st_nlink);
	} else {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) failed: rc=%d errno=%d (%s)\n",
			dirfd, relative_file, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	fd = openat(dirfd, relative_file, O_RDONLY, 0);
	if (fd >= 0) {
		dprintf(1,
			"openat(%d, \"%s\", O_RDONLY) succeeded: fd=%d errno=%d\n",
			dirfd, relative_file, fd, errno);
	} else {
		dprintf(1,
			"openat(%d, \"%s\", O_RDONLY) failed: fd=%d errno=%d (%s)\n",
			dirfd, relative_file, fd, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	/*
	 * A tracked remote regular file must not be accepted as a dirfd. The
	 * guest should now reject this locally with ENOTDIR.
	 */
	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstatat(fd, nested_missing, &st, 0);
	if (rc == -1 && errno == ENOTDIR) {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) on regular-file dirfd correctly failed: rc=%d errno=%d (%s)\n",
			fd, nested_missing, rc, errno, strerror(errno));
	} else {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) on regular-file dirfd returned unexpected result: rc=%d errno=%d (%s)\n",
			fd, nested_missing, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	pos = lseek(fd, 8, SEEK_SET);
	if (pos >= 0) {
		dprintf(1, "lseek(%d, 8, SEEK_SET) succeeded: off=%lld errno=%d\n",
			fd, (long long) pos, errno);
	} else {
		dprintf(1, "lseek(%d, 8, SEEK_SET) failed: off=%lld errno=%d (%s)\n",
			fd, (long long) pos, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstat(fd, &st);
	if (rc == 0) {
		dprintf(1,
			"fstat(%d) succeeded: rc=%d errno=%d mode=%o size=%lld nlink=%lu\n",
			fd, rc, errno, st.st_mode, (long long) st.st_size,
			(unsigned long) st.st_nlink);
	} else {
		dprintf(1, "fstat(%d) failed: rc=%d errno=%d (%s)\n", fd, rc,
			errno, strerror(errno));
		failed = 1;
		goto out;
	}

	/* Reopen for reading to show the full remote create/write/read cycle. */
	memset(buf, 0, sizeof(buf));
	errno = 0;
	nread = read(fd, buf, sizeof(buf) - 1);
	if (nread >= 0) {
		buf[nread] = '\0';
		dprintf(1,
			"read(%d, %zu) succeeded: rc=%zd errno=%d data=\"%s\"\n",
			fd, sizeof(buf) - 1, nread, errno, buf);
	} else {
		dprintf(1, "read(%d, %zu) failed: rc=%zd errno=%d (%s)\n", fd,
			sizeof(buf) - 1, nread, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	pos = lseek(fd, 0, SEEK_SET);
	if (pos >= 0) {
		dprintf(1, "lseek(%d, 0, SEEK_SET) succeeded: off=%lld errno=%d\n",
			fd, (long long) pos, errno);
	} else {
		dprintf(1, "lseek(%d, 0, SEEK_SET) failed: off=%lld errno=%d (%s)\n",
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
			"read(%d, %zu) after rewind succeeded: rc=%zd errno=%d data=\"%s\"\n",
			fd, sizeof(buf) - 1, nread, errno, buf);
	} else {
		dprintf(1,
			"read(%d, %zu) after rewind failed: rc=%zd errno=%d (%s)\n",
			fd, sizeof(buf) - 1, nread, errno, strerror(errno));
		failed = 1;
		goto out;
	}

out:
	if (fd >= 0) {
		errno = 0;
		rc = close(fd);
		if (rc == 0) {
			dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n", fd, rc,
				errno);
		} else {
			dprintf(1, "close(%d) failed: rc=%d errno=%d (%s)\n", fd, rc,
				errno, strerror(errno));
			failed = 1;
		}
	}

	if (dirfd >= 0) {
		errno = 0;
		rc = close(dirfd);
		if (rc == 0) {
			dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n", dirfd, rc,
				errno);
		} else {
			dprintf(1,
				"close(%d) failed: rc=%d errno=%d (%s)\n", dirfd, rc,
				errno, strerror(errno));
			failed = 1;
		}
	}

	return failed;
}
