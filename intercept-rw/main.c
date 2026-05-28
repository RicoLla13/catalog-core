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
	const char *file_path = "/tmp/intercept-rw.txt";
	const char *first = "written from intercept-rw";
	const char *second = " using sequential IO";
	int fd = -1;
	int rc;
	int failed = 0;
	struct stat st;
	off_t pos;
	ssize_t nread;
	ssize_t nwritten;

	(void) argc;
	(void) argv;

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

	errno = 0;
	fd = openat(AT_FDCWD, file_path, O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (fd >= 0) {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_CREAT|O_TRUNC|O_RDWR, 0644) succeeded: fd=%d errno=%d\n",
			file_path, fd, errno);
	} else {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_CREAT|O_TRUNC|O_RDWR, 0644) failed: fd=%d errno=%d (%s)\n",
			file_path, fd, errno, strerror(errno));
		return 1;
	}

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
	pos = lseek(fd, 0, SEEK_CUR);
	if (pos >= 0) {
		dprintf(1, "lseek(%d, 0, SEEK_CUR) succeeded: off=%lld errno=%d\n",
			fd, (long long) pos, errno);
	} else {
		dprintf(1, "lseek(%d, 0, SEEK_CUR) failed: off=%lld errno=%d (%s)\n",
			fd, (long long) pos, errno, strerror(errno));
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

	memset(buf, 0, sizeof(buf));
	errno = 0;
	nread = read(fd, buf, sizeof(buf) - 1);
	if (nread >= 0) {
		buf[nread] = '\0';
		dprintf(1,
			"read(%d, %zu) after seek succeeded: rc=%zd errno=%d data=\"%s\"\n",
			fd, sizeof(buf) - 1, nread, errno, buf);
	} else {
		dprintf(1,
			"read(%d, %zu) after seek failed: rc=%zd errno=%d (%s)\n",
			fd, sizeof(buf) - 1, nread, errno, strerror(errno));
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

	return failed;
}
